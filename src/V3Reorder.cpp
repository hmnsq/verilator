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
    Ast_गाठ* स्थिरांक m_nodep;

संरक्षित:
    Reorder_गाठ_Vertex(V3Graph* graphp, Ast_गाठ* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~Reorder_गाठ_Vertex() override = पूर्वनिर्धारित;
    // ACCESSORS
    // Do not make accessor for nodep(),  It may change due to
    // reordering a lower block, but we don't repair it
    std::string name() स्थिरांक override {
        std::string str = cvtToHex(m_nodep) + '\n';
        जर (AstVarScope* स्थिरांक vscp = VN_CAST(m_nodep, VarScope)) {
            str += vscp->prettyName();
        } अन्यथा {
            str += V3EmitV::debug_Verilog_For_वृक्ष(m_nodep);
            str = VString::quoteBackslash(str);
            str = VString::quoteAny(str, '"', '\\');
            str = VString::replaceSubstr(str, "\n", "\\l");
        }
        परत str;
    }
    File_ओळ* fileline() स्थिरांक override { परत nodep()->fileline(); }
    std::string dotShape() स्थिरांक override { परत VN_IS(m_nodep, VarScope) ? "ellipse" : "box"; }

सार्वजनिक:
    आभासी Ast_गाठ* nodep() स्थिरांक { परत m_nodep; }
};

वर्ग ReorderImpureVertex final : सार्वजनिक Reorder_गाठ_Vertex {
    VL_RTTI_IMPL(ReorderImpureVertex, Reorder_गाठ_Vertex)
सार्वजनिक:
    explicit ReorderImpureVertex(V3Graph* graphp, Ast_गाठ* nodep)
        : Reorder_गाठ_Vertex{graphp, nodep} {}
    ~ReorderImpureVertex() override = पूर्वनिर्धारित;
    string name() स्थिरांक override VL_MT_STABLE { परत "*IMPURE*"; }
    string dotColor() स्थिरांक override { परत "red"; }
};

वर्ग ReorderLogicVertex final : सार्वजनिक Reorder_गाठ_Vertex {
    VL_RTTI_IMPL(ReorderLogicVertex, Reorder_गाठ_Vertex)
सार्वजनिक:
    ReorderLogicVertex(V3Graph* graphp, Ast_गाठ* nodep)
        : Reorder_गाठ_Vertex{graphp, nodep} {}
    ~ReorderLogicVertex() override = पूर्वनिर्धारित;
    string dotColor() स्थिरांक override { परत "black"; }
};

वर्ग ReorderVarStdVertex final : सार्वजनिक Reorder_गाठ_Vertex {
    VL_RTTI_IMPL(ReorderVarStdVertex, Reorder_गाठ_Vertex)
सार्वजनिक:
    ReorderVarStdVertex(V3Graph* graphp, AstVarScope* nodep)
        : Reorder_गाठ_Vertex{graphp, nodep} {}
    ~ReorderVarStdVertex() override = पूर्वनिर्धारित;
    string dotColor() स्थिरांक override { परत "blue"; }
};

वर्ग ReorderVarPostVertex final : सार्वजनिक Reorder_गाठ_Vertex {
    VL_RTTI_IMPL(ReorderVarPostVertex, Reorder_गाठ_Vertex)
सार्वजनिक:
    ReorderVarPostVertex(V3Graph* graphp, AstVarScope* nodep)
        : Reorder_गाठ_Vertex{graphp, nodep} {}
    ~ReorderVarPostVertex() override = पूर्वनिर्धारित;
    string name() स्थिरांक override { परत "POST "s + Reorder_गाठ_Vertex::name(); }
    string dotColor() स्थिरांक override { परत "green"; }
};

//######################################################################
// Edge types

वर्ग ReorderEdge VL_NOT_FINAL : सार्वजनिक V3GraphEdge {
    VL_RTTI_IMPL(ReorderEdge, V3GraphEdge)
    uint32_t m_ignoreInStep = 0;  // Step number that if set to, causes this edge to be ignored
    स्थिर uint32_t s_step_अंक;  // Global step number
संरक्षित:
    स्थिर constexpr पूर्णांक WEIGHT_NORMAL = 10;
    ReorderEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष, तार्किक cutable)
        : V3GraphEdge{graphp, fromp, शीर्ष, WEIGHT_NORMAL, cutable} {}
    ~ReorderEdge() override = पूर्वनिर्धारित;

    आभासी तार्किक followScoreboard() स्थिरांक = 0;

    std::string dotStyle() स्थिरांक override {
        परत ignoreThisStep() ? "dotted" : V3GraphEdge::dotStyle();
    }

सार्वजनिक:
    // Iterator for graph functions
    स्थिर शून्य incrementStep() { ++s_step_अंक; }
    तार्किक ignoreThisStep() स्थिरांक { परत m_ignoreInStep == s_step_अंक; }
    शून्य setIgnoreThisStep() { m_ignoreInStep = s_step_अंक; }

    स्थिर तार्किक followScoreboard(स्थिरांक V3GraphEdge* edgep) {
        स्थिरांक ReorderEdge& edge = *edgep->as<ReorderEdge>();
        परत !edge.ignoreThisStep() && edge.followScoreboard();
    }
    स्थिर तार्किक followCyclic(स्थिरांक V3GraphEdge* edgep) {
        स्थिरांक ReorderEdge& edge = *edgep->as<ReorderEdge>();
        परत !edge.ignoreThisStep();
    }
};
uint32_t ReorderEdge::s_step_अंक = 0;

वर्ग ReorderPostEdge final : सार्वजनिक ReorderEdge {
    VL_RTTI_IMPL(ReorderPostEdge, ReorderEdge)
सार्वजनिक:
    ReorderPostEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष)
        : ReorderEdge{graphp, fromp, शीर्ष, CUTABLE} {}
    ~ReorderPostEdge() override = पूर्वनिर्धारित;
    तार्किक followScoreboard() स्थिरांक override { परत असत्य; }
    string dotColor() स्थिरांक override { परत "khaki"; }
};

वर्ग ReorderLVEdge final : सार्वजनिक ReorderEdge {
    VL_RTTI_IMPL(ReorderLVEdge, ReorderEdge)
सार्वजनिक:
    ReorderLVEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष)
        : ReorderEdge{graphp, fromp, शीर्ष, CUTABLE} {}
    ~ReorderLVEdge() override = पूर्वनिर्धारित;
    तार्किक followScoreboard() स्थिरांक override { परत सत्य; }
    string dotColor() स्थिरांक override { परत "yellowGreen"; }
};

वर्ग ReorderRVEdge final : सार्वजनिक ReorderEdge {
    VL_RTTI_IMPL(ReorderRVEdge, ReorderEdge)
सार्वजनिक:
    ReorderRVEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष)
        : ReorderEdge{graphp, fromp, शीर्ष, CUTABLE} {}
    ~ReorderRVEdge() override = पूर्वनिर्धारित;
    तार्किक followScoreboard() स्थिरांक override { परत सत्य; }
    string dotColor() स्थिरांक override { परत "green"; }
};

वर्ग ReorderScorebdEdge final : सार्वजनिक ReorderEdge {
    VL_RTTI_IMPL(ReorderScorebdEdge, ReorderEdge)
सार्वजनिक:
    ReorderScorebdEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष)
        : ReorderEdge{graphp, fromp, शीर्ष, CUTABLE} {}
    ~ReorderScorebdEdge() override = पूर्वनिर्धारित;
    तार्किक followScoreboard() स्थिरांक override { परत सत्य; }
    string dotColor() स्थिरांक override { परत "blue"; }
};

वर्ग ReorderStrictEdge final : सार्वजनिक ReorderEdge {
    VL_RTTI_IMPL(ReorderStrictEdge, ReorderEdge)
    // A strict order, based on the original statement order in the graph
    // The only non-cutable edge type
सार्वजनिक:
    ReorderStrictEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष)
        : ReorderEdge{graphp, fromp, शीर्ष, NOT_CUTABLE} {}
    ~ReorderStrictEdge() override = पूर्वनिर्धारित;
    तार्किक followScoreboard() स्थिरांक override { परत सत्य; }
    string dotColor() स्थिरांक override { परत "blue"; }
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
    स्थिरांक अक्षर* m_noReorderWhy = nullptr;  // Reason we can't reorder
    std::vector<ReorderLogicVertex*> m_stmtStackps;  // Current statements being tracked

    // METHODS
    शून्य cleanupBlockGraph(Ast_गाठ* nodep) {
        // Transform the graph into what we need
        UINFO(5, "ReorderBlock " << nodep);

        // Simplify graph by removing redundant edges
        m_graphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        जर (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_nodup", असत्य);

        // Mark all the logic for this step by setting Vertex::user() to true
        m_graphp->userClearVertices();
        साठी (Ast_गाठ* nextp = nodep; nextp; nextp = nextp->nextp()) {
            nextp->user3u().to<ReorderLogicVertex*>()->user(सत्य);
        }

        // New step
        ReorderEdge::incrementStep();

        // If a var vertex has only inputs, it's a input-only node,
        // and can be ignored for coloring **this block only**
        साठी (V3GraphVertex& vtx : m_graphp->vertices()) {
            जर (!vtx.outEmpty()) सुरूठेवा;
            जर (!vtx.is<ReorderVarStdVertex>()) सुरूठेवा;
            साठी (V3GraphEdge& edge : vtx.inEdges()) edge.as<ReorderEdge>()->setIgnoreThisStep();
        }

        // For reordering this single block only, mark all logic
        // vertexes not involved with this step as unimportant
        साठी (V3GraphVertex& vertex : m_graphp->vertices()) {
            जर (vertex.user()) सुरूठेवा;
            जर (!vertex.is<ReorderLogicVertex>()) सुरूठेवा;
            साठी (V3GraphEdge& edge : vertex.inEdges()) edge.as<ReorderEdge>()->setIgnoreThisStep();
            साठी (V3GraphEdge& edge : vertex.outEdges())
                edge.as<ReorderEdge>()->setIgnoreThisStep();
        }

        // Weak coloring to determine what needs to remain in order
        // This follows all step-relevant edges excluding PostEdges, which are done later
        m_graphp->weaklyConnected(&ReorderEdge::followScoreboard);

        // Add hard orderings between all nodes of same color, in the order they appeared
        std::unordered_map<uint32_t, ReorderLogicVertex*> lastOfColor;
        साठी (Ast_गाठ* currp = nodep; currp; currp = currp->nextp()) {
            ReorderLogicVertex* स्थिरांक vtxp = currp->user3u().to<ReorderLogicVertex*>();
            स्थिरांक uint32_t color = vtxp->color();
            जर (lastOfColor[color]) नवीन ReorderStrictEdge{m_graphp, lastOfColor[color], vtxp};
            lastOfColor[color] = vtxp;
        }

        // And a real ordering to get the statements into something reasonable
        // We don't care if there's cutable violations here...
        // Non-cutable violations should be impossible; as those edges are program-order
        जर (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_pre", असत्य);
        m_graphp->acyclic(&ReorderEdge::followCyclic);
        m_graphp->rank(&ReorderEdge::followCyclic);  // Or order(), but that's more expensive
        जर (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_opt", असत्य);
    }

    शून्य reorderBlock(Ast_गाठ* nodep) {
        // Reorder statements in the completed graph

        // Map the rank numbers into nodes they associate with
        std::multimap<uint32_t, Ast_गाठ*> rankMap;
        पूर्णांक currOrder = 0;  // Existing sequence number of assignment
        साठी (Ast_गाठ* currp = nodep; currp; currp = currp->nextp()) {
            स्थिरांक ReorderLogicVertex* स्थिरांक vtxp = currp->user3u().to<ReorderLogicVertex*>();
            rankMap.emplace(vtxp->rank(), currp);
            currp->user4(++currOrder);  // Record current ordering
        }

        // Is the current ordering OK?
        तार्किक leaveAlone = सत्य;
        uint64_t newOrder = 0;  // New sequence number of assignment
        साठी (स्थिरांक स्वतः& item : rankMap) {
            स्थिरांक Ast_गाठ* स्थिरांक nextp = item.second;
            जर (++newOrder != nextp->user4()) leaveAlone = असत्य;
        }
        जर (leaveAlone) {
            UINFO(6, "   No changes");
            परत;
        }

        VNRelinker replaceHandle;  // Where to add the list
        Ast_गाठ* newListp = nullptr;
        साठी (स्थिरांक स्वतः& item : rankMap) {
            Ast_गाठ* स्थिरांक nextp = item.second;
            UINFO(6, "   New order: " << nextp);
            nextp->unlinkFrBack(nextp == nodep ? &replaceHandle : nullptr);
            newListp = Ast_गाठ::addNext(newListp, nextp);
        }
        replaceHandle.relink(newListp);
    }

    शून्य processBlock(Ast_गाठ* nodep) {
        जर (m_noReorderWhy) परत;

        // Empty lists are ignorable
        जर (!nodep) परत;
        UASSERT_OBJ(nodep->firstAbovep(), nodep, "Node passed is in not head of list");
        UASSERT_OBJ(!nodep->user3p(), nodep, "Should not have a logic vertex");

        // It nothing to reorder with, just iterate
        जर (!nodep->nextp()) {
            iterate(nodep);
            परत;
        }

        // Process it
        UINFO(9, "  processBlock " << nodep);

        // Iterate across current block, making the scoreboard
        साठी (Ast_गाठ* currp = nodep; currp; currp = currp->nextp()) {
            // Create the logic vertex for this statement
            UASSERT_OBJ(!currp->user3p(), currp, "user3p should not be set");
            ReorderLogicVertex* स्थिरांक vtxp = नवीन ReorderLogicVertex{m_graphp, currp};
            currp->user3p(vtxp);

            // Visit the statement - this can recursively reorder sub statements
            m_stmtStackps.ढकला_back(vtxp);
            iterate(currp);
            m_stmtStackps.काढा_back();
        }

        जर (m_noReorderWhy) {  // Jump or something nasty
            UINFO(9, "  NoReorderBlock because " << m_noReorderWhy);
            परत;
        }

        // Reorder statements in this block
        cleanupBlockGraph(nodep);
        reorderBlock(nodep);

        // 'nodep' might no longer be the head of the list, rewind
        पर्यंत (nodep->backp()->nextp() == nodep) nodep = nodep->backp();

        // Delete vertexes and edges only applying to this block
        साठी (Ast_गाठ* currp = nodep; currp; currp = currp->nextp()) {
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

        स्थिरांक VNUser1InUse user1InUse;
        स्थिरांक VNUser2InUse user2InUse;
        स्थिरांक VNUser3InUse user3InUse;
        स्थिरांक VNUser4InUse user4InUse;

        UINFOTREE(9, nodep, "", "alwIn:");
        processBlock(nodep->stmtsp());
        UINFOTREE(9, nodep, "", "alwOut");

        m_stmtStackps.clear();
    }

    शून्य visit(Ast_गाठ_If* nodep) override {
        जर (!m_graphp || m_noReorderWhy) परत;
        iterateAndNextNull(nodep->condp());
        processBlock(nodep->thensp());
        processBlock(nodep->elsesp());
    }

    शून्य visit(AstJumpGo* nodep) override {
        जर (!m_graphp || m_noReorderWhy) परत;
        m_noReorderWhy = "JumpGo";
        iterateChildren(nodep);
    }

    शून्य visit(Ast_गाठ_CCall* nodep) override {
        जर (!m_graphp || m_noReorderWhy) परत;
        // The callee is not inlined, so the variables it reads and writes carry no VarRef at
        // the call site, and the scoreboard would let assignments to them move across the
        // call. Treat any such call as a barrier.
        m_noReorderWhy = "CCall";
        iterateChildren(nodep);
    }

    शून्य visit(Ast_अभिव्यक्ती_Stmt* nodep) override {
        जर (!m_graphp || m_noReorderWhy) परत;
        VL_RESTORER(m_inDly);
        m_inDly = असत्य;
        iterateChildren(nodep);
    }

    शून्य visit(AstAssignDly* nodep) override {
        जर (!m_graphp || m_noReorderWhy) परत;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        जर (Ast_गाठ* स्थिरांक controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_inDly);
        m_inDly = सत्य;
        iterate(nodep->lhsp());
    }

    शून्य visit(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        जर (m_graphp) m_noReorderWhy = "Handle";
        visit(static_cast<Ast_गाठ*>(nodep));
    }

    शून्य visit(AstVarRef* nodep) override {
        जर (!m_graphp || m_noReorderWhy) परत;
        जर (m_stmtStackps.empty()) परत;

        // Reads of constants can be ignored - TODO: This should be "constexpr", not run-time const
        जर (nodep->varp()->isConst()) परत;

        // SPEEDUP: We add duplicate edges, that should be fixed

        AstVarScope* स्थिरांक vscp = nodep->varScopep();

        // Create vertexes for variable
        जर (!vscp->user1p()) vscp->user1p(नवीन ReorderVarStdVertex{m_graphp, vscp});
        ReorderVarStdVertex* स्थिरांक vstdp = vscp->user1u().to<ReorderVarStdVertex*>();

        // Variable is read
        जर (nodep->access().isReadOnly()) {
            साठी (ReorderLogicVertex* स्थिरांक vtxp : m_stmtStackps) {
                नवीन ReorderRVEdge{m_graphp, vtxp, vstdp};
            }
            परत;
        }

        // Variable is written, not NBA
        जर (!m_inDly) {
            साठी (ReorderLogicVertex* स्थिरांक vtxp : m_stmtStackps) {
                नवीन ReorderLVEdge{m_graphp, vstdp, vtxp};
            }
            परत;
        }

        // Variable is written by NBA
        जर (!vscp->user2p()) {
            ReorderVarPostVertex* स्थिरांक vpostp = नवीन ReorderVarPostVertex{m_graphp, vscp};
            vscp->user2p(vpostp);
            नवीन ReorderPostEdge{m_graphp, vstdp, vpostp};
        }
        ReorderVarPostVertex* स्थिरांक vpostp = vscp->user2u().to<ReorderVarPostVertex*>();
        साठी (ReorderLogicVertex* स्थिरांक vtxp : m_stmtStackps) {
            नवीन ReorderLVEdge{m_graphp, vpostp, vtxp};
        }
    }

    शून्य visit(Ast_गाठ* nodep) override {
        // Outside AstAlways, just descend
        जर (!m_graphp) {
            iterateChildren(nodep);
            परत;
        }

        // Early exit if decided not to reorder
        जर (m_noReorderWhy) परत;

        // Timing control prevents reordering
        जर (nodep->isTimingControl()) {
            m_noReorderWhy = "TimingControl";
            परत;
        }

        // Order all impure statements with other impure statements
        जर (!nodep->isPure()) {
            जर (!m_impureVtxp) m_impureVtxp = नवीन ReorderImpureVertex{m_graphp, nodep};
            // This edge is only used to find weakly connected components, so one edge is enough
            साठी (ReorderLogicVertex* स्थिरांक vtxp : m_stmtStackps) {
                नवीन ReorderScorebdEdge{m_graphp, m_impureVtxp, vtxp};
            }
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
सार्वजनिक:
    explicit ReorderVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReorderVisitor() override = पूर्वनिर्धारित;
};

}  // namespace

//######################################################################
// V3Reorder class functions

शून्य V3Reorder::reorderAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReorderVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("reorder", 0, dump_वृक्ष_Either_Level() >= 3);
}
