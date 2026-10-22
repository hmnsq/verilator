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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Reorder.h"

#شامل "V3EmitV.h"
#شامل "V3Graph.h"
#شامل "V3Stats.h"

#شامل <string>
#شامل <unordered_map>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

فضاینام {

//######################################################################
// Support classes

رده Reorder_گره_Vertex VL_NOT_FINAL : عمومی V3GraphVertex {
    VL_RTTI_IMPL(Reorder_گره_Vertex, V3GraphVertex)
    Ast_گره* ثابت m_nodep;

محافظتشده:
    Reorder_گره_Vertex(V3Graph* graphp, Ast_گره* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~Reorder_گره_Vertex() override = پیشفرض;
    // ACCESSORS
    // Do not make accessor for nodep(),  It may change due to
    // reordering a lower block, but we don't repair it
    std::string name() ثابت override {
        std::string str = cvtToHex(m_nodep) + '\n';
        اگر (AstVarScope* ثابت vscp = VN_CAST(m_nodep, VarScope)) {
            str += vscp->prettyName();
        } وگرنه {
            str += V3EmitV::debug_Verilog_For_درخت(m_nodep);
            str = VString::quoteBackslash(str);
            str = VString::quoteAny(str, '"', '\\');
            str = VString::replaceSubstr(str, "\n", "\\l");
        }
        بازگشت str;
    }
    File_سطر* fileline() ثابت override { بازگشت nodep()->fileline(); }
    std::string dotShape() ثابت override { بازگشت VN_IS(m_nodep, VarScope) ? "ellipse" : "box"; }

عمومی:
    مجازی Ast_گره* nodep() ثابت { بازگشت m_nodep; }
};

رده ReorderImpureVertex final : عمومی Reorder_گره_Vertex {
    VL_RTTI_IMPL(ReorderImpureVertex, Reorder_گره_Vertex)
عمومی:
    explicit ReorderImpureVertex(V3Graph* graphp, Ast_گره* nodep)
        : Reorder_گره_Vertex{graphp, nodep} {}
    ~ReorderImpureVertex() override = پیشفرض;
    string name() ثابت override VL_MT_STABLE { بازگشت "*IMPURE*"; }
    string dotColor() ثابت override { بازگشت "red"; }
};

رده ReorderLogicVertex final : عمومی Reorder_گره_Vertex {
    VL_RTTI_IMPL(ReorderLogicVertex, Reorder_گره_Vertex)
عمومی:
    ReorderLogicVertex(V3Graph* graphp, Ast_گره* nodep)
        : Reorder_گره_Vertex{graphp, nodep} {}
    ~ReorderLogicVertex() override = پیشفرض;
    string dotColor() ثابت override { بازگشت "black"; }
};

رده ReorderVarStdVertex final : عمومی Reorder_گره_Vertex {
    VL_RTTI_IMPL(ReorderVarStdVertex, Reorder_گره_Vertex)
عمومی:
    ReorderVarStdVertex(V3Graph* graphp, AstVarScope* nodep)
        : Reorder_گره_Vertex{graphp, nodep} {}
    ~ReorderVarStdVertex() override = پیشفرض;
    string dotColor() ثابت override { بازگشت "blue"; }
};

رده ReorderVarPostVertex final : عمومی Reorder_گره_Vertex {
    VL_RTTI_IMPL(ReorderVarPostVertex, Reorder_گره_Vertex)
عمومی:
    ReorderVarPostVertex(V3Graph* graphp, AstVarScope* nodep)
        : Reorder_گره_Vertex{graphp, nodep} {}
    ~ReorderVarPostVertex() override = پیشفرض;
    string name() ثابت override { بازگشت "POST "s + Reorder_گره_Vertex::name(); }
    string dotColor() ثابت override { بازگشت "green"; }
};

//######################################################################
// Edge types

رده ReorderEdge VL_NOT_FINAL : عمومی V3GraphEdge {
    VL_RTTI_IMPL(ReorderEdge, V3GraphEdge)
    uint32_t m_ignoreInStep = 0;  // Step number that if set to, causes this edge to be ignored
    ایستا uint32_t s_step_رقم;  // Global step number
محافظتشده:
    ایستا constexpr صحیح WEIGHT_NORMAL = 10;
    ReorderEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* بالاترین, منطقی cutable)
        : V3GraphEdge{graphp, fromp, بالاترین, WEIGHT_NORMAL, cutable} {}
    ~ReorderEdge() override = پیشفرض;

    مجازی منطقی followScoreboard() ثابت = 0;

    std::string dotStyle() ثابت override {
        بازگشت ignoreThisStep() ? "dotted" : V3GraphEdge::dotStyle();
    }

عمومی:
    // Iterator for graph functions
    ایستا تهی incrementStep() { ++s_step_رقم; }
    منطقی ignoreThisStep() ثابت { بازگشت m_ignoreInStep == s_step_رقم; }
    تهی setIgnoreThisStep() { m_ignoreInStep = s_step_رقم; }

    ایستا منطقی followScoreboard(ثابت V3GraphEdge* edgep) {
        ثابت ReorderEdge& edge = *edgep->as<ReorderEdge>();
        بازگشت !edge.ignoreThisStep() && edge.followScoreboard();
    }
    ایستا منطقی followCyclic(ثابت V3GraphEdge* edgep) {
        ثابت ReorderEdge& edge = *edgep->as<ReorderEdge>();
        بازگشت !edge.ignoreThisStep();
    }
};
uint32_t ReorderEdge::s_step_رقم = 0;

رده ReorderPostEdge final : عمومی ReorderEdge {
    VL_RTTI_IMPL(ReorderPostEdge, ReorderEdge)
عمومی:
    ReorderPostEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* بالاترین)
        : ReorderEdge{graphp, fromp, بالاترین, CUTABLE} {}
    ~ReorderPostEdge() override = پیشفرض;
    منطقی followScoreboard() ثابت override { بازگشت نادرست; }
    string dotColor() ثابت override { بازگشت "khaki"; }
};

رده ReorderLVEdge final : عمومی ReorderEdge {
    VL_RTTI_IMPL(ReorderLVEdge, ReorderEdge)
عمومی:
    ReorderLVEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* بالاترین)
        : ReorderEdge{graphp, fromp, بالاترین, CUTABLE} {}
    ~ReorderLVEdge() override = پیشفرض;
    منطقی followScoreboard() ثابت override { بازگشت درست; }
    string dotColor() ثابت override { بازگشت "yellowGreen"; }
};

رده ReorderRVEdge final : عمومی ReorderEdge {
    VL_RTTI_IMPL(ReorderRVEdge, ReorderEdge)
عمومی:
    ReorderRVEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* بالاترین)
        : ReorderEdge{graphp, fromp, بالاترین, CUTABLE} {}
    ~ReorderRVEdge() override = پیشفرض;
    منطقی followScoreboard() ثابت override { بازگشت درست; }
    string dotColor() ثابت override { بازگشت "green"; }
};

رده ReorderScorebdEdge final : عمومی ReorderEdge {
    VL_RTTI_IMPL(ReorderScorebdEdge, ReorderEdge)
عمومی:
    ReorderScorebdEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* بالاترین)
        : ReorderEdge{graphp, fromp, بالاترین, CUTABLE} {}
    ~ReorderScorebdEdge() override = پیشفرض;
    منطقی followScoreboard() ثابت override { بازگشت درست; }
    string dotColor() ثابت override { بازگشت "blue"; }
};

رده ReorderStrictEdge final : عمومی ReorderEdge {
    VL_RTTI_IMPL(ReorderStrictEdge, ReorderEdge)
    // A strict order, based on the original statement order in the graph
    // The only non-cutable edge type
عمومی:
    ReorderStrictEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* بالاترین)
        : ReorderEdge{graphp, fromp, بالاترین, NOT_CUTABLE} {}
    ~ReorderStrictEdge() override = پیشفرض;
    منطقی followScoreboard() ثابت override { بازگشت درست; }
    string dotColor() ثابت override { بازگشت "blue"; }
};

//######################################################################
// Reorder class functions

رده ReorderVisitor final : عمومی VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> ReorderVarStdVertex*.  Usage var, 0=not set yet
    // AstVarScope::user2p  -> ReorderVarPostVertex*.  Delayed assignment var, 0=not set yet
    // Ast*::user3p         -> ReorderLogicVertex*.  Statement (temporary only)
    // Ast*::user4          -> int.  Current ordering number (reorderBlock usage)

    // STATE
    V3Graph* m_graphp = nullptr;  // Scoreboard of var usages/dependencies
    ReorderImpureVertex* m_impureVtxp = nullptr;  // Element specifying PLI ordering
    منطقی m_inDly = نادرست;  // Inside ASSIGNDLY
    ثابت نویسه* m_noReorderWhy = nullptr;  // Reason we can't reorder
    std::vector<ReorderLogicVertex*> m_stmtStackps;  // Current statements being tracked

    // METHODS
    تهی cleanupBlockGraph(Ast_گره* nodep) {
        // Transform the graph into what we need
        UINFO(5, "ReorderBlock " << nodep);

        // Simplify graph by removing redundant edges
        m_graphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        اگر (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_nodup", نادرست);

        // Mark all the logic for this step by setting Vertex::user() to true
        m_graphp->userClearVertices();
        برای (Ast_گره* nextp = nodep; nextp; nextp = nextp->nextp()) {
            nextp->user3u().to<ReorderLogicVertex*>()->user(درست);
        }

        // New step
        ReorderEdge::incrementStep();

        // If a var vertex has only inputs, it's a input-only node,
        // and can be ignored for coloring **this block only**
        برای (V3GraphVertex& vtx : m_graphp->vertices()) {
            اگر (!vtx.outEmpty()) ادامه;
            اگر (!vtx.is<ReorderVarStdVertex>()) ادامه;
            برای (V3GraphEdge& edge : vtx.inEdges()) edge.as<ReorderEdge>()->setIgnoreThisStep();
        }

        // For reordering this single block only, mark all logic
        // vertexes not involved with this step as unimportant
        برای (V3GraphVertex& vertex : m_graphp->vertices()) {
            اگر (vertex.user()) ادامه;
            اگر (!vertex.is<ReorderLogicVertex>()) ادامه;
            برای (V3GraphEdge& edge : vertex.inEdges()) edge.as<ReorderEdge>()->setIgnoreThisStep();
            برای (V3GraphEdge& edge : vertex.outEdges())
                edge.as<ReorderEdge>()->setIgnoreThisStep();
        }

        // Weak coloring to determine what needs to remain in order
        // This follows all step-relevant edges excluding PostEdges, which are done later
        m_graphp->weaklyConnected(&ReorderEdge::followScoreboard);

        // Add hard orderings between all nodes of same color, in the order they appeared
        std::unordered_map<uint32_t, ReorderLogicVertex*> lastOfColor;
        برای (Ast_گره* currp = nodep; currp; currp = currp->nextp()) {
            ReorderLogicVertex* ثابت vtxp = currp->user3u().to<ReorderLogicVertex*>();
            ثابت uint32_t color = vtxp->color();
            اگر (lastOfColor[color]) جدید ReorderStrictEdge{m_graphp, lastOfColor[color], vtxp};
            lastOfColor[color] = vtxp;
        }

        // And a real ordering to get the statements into something reasonable
        // We don't care if there's cutable violations here...
        // Non-cutable violations should be impossible; as those edges are program-order
        اگر (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_pre", نادرست);
        m_graphp->acyclic(&ReorderEdge::followCyclic);
        m_graphp->rank(&ReorderEdge::followCyclic);  // Or order(), but that's more expensive
        اگر (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_opt", نادرست);
    }

    تهی reorderBlock(Ast_گره* nodep) {
        // Reorder statements in the completed graph

        // Map the rank numbers into nodes they associate with
        std::multimap<uint32_t, Ast_گره*> rankMap;
        صحیح currOrder = 0;  // Existing sequence number of assignment
        برای (Ast_گره* currp = nodep; currp; currp = currp->nextp()) {
            ثابت ReorderLogicVertex* ثابت vtxp = currp->user3u().to<ReorderLogicVertex*>();
            rankMap.emplace(vtxp->rank(), currp);
            currp->user4(++currOrder);  // Record current ordering
        }

        // Is the current ordering OK?
        منطقی leaveAlone = درست;
        uint64_t newOrder = 0;  // New sequence number of assignment
        برای (ثابت auto& item : rankMap) {
            ثابت Ast_گره* ثابت nextp = item.second;
            اگر (++newOrder != nextp->user4()) leaveAlone = نادرست;
        }
        اگر (leaveAlone) {
            UINFO(6, "   No changes");
            بازگشت;
        }

        VNRelinker replaceHandle;  // Where to add the list
        Ast_گره* newListp = nullptr;
        برای (ثابت auto& item : rankMap) {
            Ast_گره* ثابت nextp = item.second;
            UINFO(6, "   New order: " << nextp);
            nextp->unlinkFrBack(nextp == nodep ? &replaceHandle : nullptr);
            newListp = Ast_گره::addNext(newListp, nextp);
        }
        replaceHandle.relink(newListp);
    }

    تهی processBlock(Ast_گره* nodep) {
        اگر (m_noReorderWhy) بازگشت;

        // Empty lists are ignorable
        اگر (!nodep) بازگشت;
        UASSERT_OBJ(nodep->firstAbovep(), nodep, "Node passed is in not head of list");
        UASSERT_OBJ(!nodep->user3p(), nodep, "Should not have a logic vertex");

        // It nothing to reorder with, just iterate
        اگر (!nodep->nextp()) {
            iterate(nodep);
            بازگشت;
        }

        // Process it
        UINFO(9, "  processBlock " << nodep);

        // Iterate across current block, making the scoreboard
        برای (Ast_گره* currp = nodep; currp; currp = currp->nextp()) {
            // Create the logic vertex for this statement
            UASSERT_OBJ(!currp->user3p(), currp, "user3p should not be set");
            ReorderLogicVertex* ثابت vtxp = جدید ReorderLogicVertex{m_graphp, currp};
            currp->user3p(vtxp);

            // Visit the statement - this can recursively reorder sub statements
            m_stmtStackps.افزودن_back(vtxp);
            iterate(currp);
            m_stmtStackps.برداشت_back();
        }

        اگر (m_noReorderWhy) {  // Jump or something nasty
            UINFO(9, "  NoReorderBlock because " << m_noReorderWhy);
            بازگشت;
        }

        // Reorder statements in this block
        cleanupBlockGraph(nodep);
        reorderBlock(nodep);

        // 'nodep' might no longer be the head of the list, rewind
        وقتی (nodep->backp()->nextp() == nodep) nodep = nodep->backp();

        // Delete vertexes and edges only applying to this block
        برای (Ast_گره* currp = nodep; currp; currp = currp->nextp()) {
            currp->user3u().to<ReorderLogicVertex*>()->unlinkDelete(m_graphp);
            currp->user3p(nullptr);
        }
    }

    // VISITORS
    تهی visit(AstAlways* nodep) override {
        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_inDly);
        VL_RESTORER(m_noReorderWhy);

        V3Graph graph;
        m_graphp = &graph;
        m_impureVtxp = nullptr;
        m_inDly = نادرست;
        m_noReorderWhy = nullptr;

        ثابت VNUser1InUse user1InUse;
        ثابت VNUser2InUse user2InUse;
        ثابت VNUser3InUse user3InUse;
        ثابت VNUser4InUse user4InUse;

        UINFOTREE(9, nodep, "", "alwIn:");
        processBlock(nodep->stmtsp());
        UINFOTREE(9, nodep, "", "alwOut");

        m_stmtStackps.clear();
    }

    تهی visit(Ast_گره_If* nodep) override {
        اگر (!m_graphp || m_noReorderWhy) بازگشت;
        iterateAndNextNull(nodep->condp());
        processBlock(nodep->thensp());
        processBlock(nodep->elsesp());
    }

    تهی visit(AstJumpGo* nodep) override {
        اگر (!m_graphp || m_noReorderWhy) بازگشت;
        m_noReorderWhy = "JumpGo";
        iterateChildren(nodep);
    }

    تهی visit(Ast_گره_CCall* nodep) override {
        اگر (!m_graphp || m_noReorderWhy) بازگشت;
        // The callee is not inlined, so the variables it reads and writes carry no VarRef at
        // the call site, and the scoreboard would let assignments to them move across the
        // call. Treat any such call as a barrier.
        m_noReorderWhy = "CCall";
        iterateChildren(nodep);
    }

    تهی visit(Ast_عبارت_Stmt* nodep) override {
        اگر (!m_graphp || m_noReorderWhy) بازگشت;
        VL_RESTORER(m_inDly);
        m_inDly = نادرست;
        iterateChildren(nodep);
    }

    تهی visit(AstAssignDly* nodep) override {
        اگر (!m_graphp || m_noReorderWhy) بازگشت;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        اگر (Ast_گره* ثابت controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_inDly);
        m_inDly = درست;
        iterate(nodep->lhsp());
    }

    تهی visit(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        اگر (m_graphp) m_noReorderWhy = "Handle";
        visit(static_cast<Ast_گره*>(nodep));
    }

    تهی visit(AstVarRef* nodep) override {
        اگر (!m_graphp || m_noReorderWhy) بازگشت;
        اگر (m_stmtStackps.empty()) بازگشت;

        // Reads of constants can be ignored - TODO: This should be "constexpr", not run-time const
        اگر (nodep->varp()->isConst()) بازگشت;

        // SPEEDUP: We add duplicate edges, that should be fixed

        AstVarScope* ثابت vscp = nodep->varScopep();

        // Create vertexes for variable
        اگر (!vscp->user1p()) vscp->user1p(جدید ReorderVarStdVertex{m_graphp, vscp});
        ReorderVarStdVertex* ثابت vstdp = vscp->user1u().to<ReorderVarStdVertex*>();

        // Variable is read
        اگر (nodep->access().isReadOnly()) {
            برای (ReorderLogicVertex* ثابت vtxp : m_stmtStackps) {
                جدید ReorderRVEdge{m_graphp, vtxp, vstdp};
            }
            بازگشت;
        }

        // Variable is written, not NBA
        اگر (!m_inDly) {
            برای (ReorderLogicVertex* ثابت vtxp : m_stmtStackps) {
                جدید ReorderLVEdge{m_graphp, vstdp, vtxp};
            }
            بازگشت;
        }

        // Variable is written by NBA
        اگر (!vscp->user2p()) {
            ReorderVarPostVertex* ثابت vpostp = جدید ReorderVarPostVertex{m_graphp, vscp};
            vscp->user2p(vpostp);
            جدید ReorderPostEdge{m_graphp, vstdp, vpostp};
        }
        ReorderVarPostVertex* ثابت vpostp = vscp->user2u().to<ReorderVarPostVertex*>();
        برای (ReorderLogicVertex* ثابت vtxp : m_stmtStackps) {
            جدید ReorderLVEdge{m_graphp, vpostp, vtxp};
        }
    }

    تهی visit(Ast_گره* nodep) override {
        // Outside AstAlways, just descend
        اگر (!m_graphp) {
            iterateChildren(nodep);
            بازگشت;
        }

        // Early exit if decided not to reorder
        اگر (m_noReorderWhy) بازگشت;

        // Timing control prevents reordering
        اگر (nodep->isTimingControl()) {
            m_noReorderWhy = "TimingControl";
            بازگشت;
        }

        // Order all impure statements with other impure statements
        اگر (!nodep->isPure()) {
            اگر (!m_impureVtxp) m_impureVtxp = جدید ReorderImpureVertex{m_graphp, nodep};
            // This edge is only used to find weakly connected components, so one edge is enough
            برای (ReorderLogicVertex* ثابت vtxp : m_stmtStackps) {
                جدید ReorderScorebdEdge{m_graphp, m_impureVtxp, vtxp};
            }
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
عمومی:
    explicit ReorderVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReorderVisitor() override = پیشفرض;
};

}  // namespace

//######################################################################
// V3Reorder class functions

تهی V3Reorder::reorderAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReorderVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("reorder", 0, dump_درخت_Either_Level() >= 3);
}
