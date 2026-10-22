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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Reorder.h"

#అన్తర్భావ "V3EmitV.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <string>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

నామాకాశ {

//######################################################################
// Support classes

తరగతి Reorder_కణుపు_శీర్షం VL_NOT_FINAL : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Reorder_కణుపు_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
    Ast_కణుపు* స్థిరరాశి m_nodep;

రక్షిత:
    Reorder_కణుపు_శీర్షం(V3_లేఖాచిత్రం* graphp, Ast_కణుపు* nodep)
        : V3_లేఖాచిత్రం_శీర్షం{graphp}
        , m_nodep{nodep} {}
    ~Reorder_కణుపు_శీర్షం() override = అప్రమేయ;
    // ACCESSORS
    // Do not make accessor for nodep(),  It may change due to
    // reordering a lower block, but we don't repair it
    std::string name() స్థిరరాశి override {
        std::string str = cvt_గమ్యం_Hex(m_nodep) + '\n';
        ఒకవేళ (AstVarScope* స్థిరరాశి vscp = VN_CAST(m_nodep, VarScope)) {
            str += vscp->prettyName();
        } లేకపోతే {
            str += V3EmitV::debug_Verilog_For_వృక్షం(m_nodep);
            str = VString::quoteBackslash(str);
            str = VString::quoteAny(str, '"', '\\');
            str = VString::replaceSubstr(str, "\n", "\\l");
        }
        తిరిగి str;
    }
    File_పంక్తి* fileline() స్థిరరాశి override { తిరిగి nodep()->fileline(); }
    std::string dotShape() స్థిరరాశి override { తిరిగి VN_IS(m_nodep, VarScope) ? "ellipse" : "box"; }

బహిరంగ:
    మాయా Ast_కణుపు* nodep() స్థిరరాశి { తిరిగి m_nodep; }
};

తరగతి Reorder_Impure_శీర్షం final : బహిరంగ Reorder_కణుపు_శీర్షం {
    VL_RTTI_IMPL(Reorder_Impure_శీర్షం, Reorder_కణుపు_శీర్షం)
బహిరంగ:
    స్పష్టమైన Reorder_Impure_శీర్షం(V3_లేఖాచిత్రం* graphp, Ast_కణుపు* nodep)
        : Reorder_కణుపు_శీర్షం{graphp, nodep} {}
    ~Reorder_Impure_శీర్షం() override = అప్రమేయ;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి "*IMPURE*"; }
    string dotColor() స్థిరరాశి override { తిరిగి "red"; }
};

తరగతి Reorder_Logic_శీర్షం final : బహిరంగ Reorder_కణుపు_శీర్షం {
    VL_RTTI_IMPL(Reorder_Logic_శీర్షం, Reorder_కణుపు_శీర్షం)
బహిరంగ:
    Reorder_Logic_శీర్షం(V3_లేఖాచిత్రం* graphp, Ast_కణుపు* nodep)
        : Reorder_కణుపు_శీర్షం{graphp, nodep} {}
    ~Reorder_Logic_శీర్షం() override = అప్రమేయ;
    string dotColor() స్థిరరాశి override { తిరిగి "black"; }
};

తరగతి Reorder_Var_Std_శీర్షం final : బహిరంగ Reorder_కణుపు_శీర్షం {
    VL_RTTI_IMPL(Reorder_Var_Std_శీర్షం, Reorder_కణుపు_శీర్షం)
బహిరంగ:
    Reorder_Var_Std_శీర్షం(V3_లేఖాచిత్రం* graphp, AstVarScope* nodep)
        : Reorder_కణుపు_శీర్షం{graphp, nodep} {}
    ~Reorder_Var_Std_శీర్షం() override = అప్రమేయ;
    string dotColor() స్థిరరాశి override { తిరిగి "blue"; }
};

తరగతి Reorder_Var_Post_శీర్షం final : బహిరంగ Reorder_కణుపు_శీర్షం {
    VL_RTTI_IMPL(Reorder_Var_Post_శీర్షం, Reorder_కణుపు_శీర్షం)
బహిరంగ:
    Reorder_Var_Post_శీర్షం(V3_లేఖాచిత్రం* graphp, AstVarScope* nodep)
        : Reorder_కణుపు_శీర్షం{graphp, nodep} {}
    ~Reorder_Var_Post_శీర్షం() override = అప్రమేయ;
    string name() స్థిరరాశి override { తిరిగి "POST "s + Reorder_కణుపు_శీర్షం::name(); }
    string dotColor() స్థిరరాశి override { తిరిగి "green"; }
};

//######################################################################
// Edge types

తరగతి Reorder_అంచు VL_NOT_FINAL : బహిరంగ V3_లేఖాచిత్రం_అంచు {
    VL_RTTI_IMPL(Reorder_అంచు, V3_లేఖాచిత్రం_అంచు)
    uint32_t m_ignore_లోపల_అడుగు = 0;  // Step number that if set to, causes this edge to be ignored
    స్థిర uint32_t s_అడుగు_అంకె;  // Global step number
రక్షిత:
    స్థిర constexpr పూర్ణాంకం బరువు_సాధారణ = 10;
    Reorder_అంచు(V3_లేఖాచిత్రం* graphp, V3_లేఖాచిత్రం_శీర్షం* fromp, V3_లేఖాచిత్రం_శీర్షం* శిఖరం, తార్కిక cutable)
        : V3_లేఖాచిత్రం_అంచు{graphp, fromp, శిఖరం, బరువు_సాధారణ, cutable} {}
    ~Reorder_అంచు() override = అప్రమేయ;

    మాయా తార్కిక followScoreboard() స్థిరరాశి = 0;

    std::string dotStyle() స్థిరరాశి override {
        తిరిగి ignore_This_అడుగు() ? "dotted" : V3_లేఖాచిత్రం_అంచు::dotStyle();
    }

బహిరంగ:
    // Iterator for graph functions
    స్థిర శూన్య increment_అడుగు() { ++s_అడుగు_అంకె; }
    తార్కిక ignore_This_అడుగు() స్థిరరాశి { తిరిగి m_ignore_లోపల_అడుగు == s_అడుగు_అంకె; }
    శూన్య సమితి_Ignore_This_అడుగు() { m_ignore_లోపల_అడుగు = s_అడుగు_అంకె; }

    స్థిర తార్కిక followScoreboard(స్థిరరాశి V3_లేఖాచిత్రం_అంచు* edgep) {
        స్థిరరాశి Reorder_అంచు& అంచు = *edgep->as<Reorder_అంచు>();
        తిరిగి !అంచు.ignore_This_అడుగు() && అంచు.followScoreboard();
    }
    స్థిర తార్కిక followCyclic(స్థిరరాశి V3_లేఖాచిత్రం_అంచు* edgep) {
        స్థిరరాశి Reorder_అంచు& అంచు = *edgep->as<Reorder_అంచు>();
        తిరిగి !అంచు.ignore_This_అడుగు();
    }
};
uint32_t Reorder_అంచు::s_అడుగు_అంకె = 0;

తరగతి Reorder_Post_అంచు final : బహిరంగ Reorder_అంచు {
    VL_RTTI_IMPL(Reorder_Post_అంచు, Reorder_అంచు)
బహిరంగ:
    Reorder_Post_అంచు(V3_లేఖాచిత్రం* graphp, V3_లేఖాచిత్రం_శీర్షం* fromp, V3_లేఖాచిత్రం_శీర్షం* శిఖరం)
        : Reorder_అంచు{graphp, fromp, శిఖరం, CUTABLE} {}
    ~Reorder_Post_అంచు() override = అప్రమేయ;
    తార్కిక followScoreboard() స్థిరరాశి override { తిరిగి అబద్ధం; }
    string dotColor() స్థిరరాశి override { తిరిగి "khaki"; }
};

తరగతి ReorderLVEdge final : బహిరంగ Reorder_అంచు {
    VL_RTTI_IMPL(ReorderLVEdge, Reorder_అంచు)
బహిరంగ:
    ReorderLVEdge(V3_లేఖాచిత్రం* graphp, V3_లేఖాచిత్రం_శీర్షం* fromp, V3_లేఖాచిత్రం_శీర్షం* శిఖరం)
        : Reorder_అంచు{graphp, fromp, శిఖరం, CUTABLE} {}
    ~ReorderLVEdge() override = అప్రమేయ;
    తార్కిక followScoreboard() స్థిరరాశి override { తిరిగి నిజం; }
    string dotColor() స్థిరరాశి override { తిరిగి "yellowGreen"; }
};

తరగతి ReorderRVEdge final : బహిరంగ Reorder_అంచు {
    VL_RTTI_IMPL(ReorderRVEdge, Reorder_అంచు)
బహిరంగ:
    ReorderRVEdge(V3_లేఖాచిత్రం* graphp, V3_లేఖాచిత్రం_శీర్షం* fromp, V3_లేఖాచిత్రం_శీర్షం* శిఖరం)
        : Reorder_అంచు{graphp, fromp, శిఖరం, CUTABLE} {}
    ~ReorderRVEdge() override = అప్రమేయ;
    తార్కిక followScoreboard() స్థిరరాశి override { తిరిగి నిజం; }
    string dotColor() స్థిరరాశి override { తిరిగి "green"; }
};

తరగతి Reorder_Scorebd_అంచు final : బహిరంగ Reorder_అంచు {
    VL_RTTI_IMPL(Reorder_Scorebd_అంచు, Reorder_అంచు)
బహిరంగ:
    Reorder_Scorebd_అంచు(V3_లేఖాచిత్రం* graphp, V3_లేఖాచిత్రం_శీర్షం* fromp, V3_లేఖాచిత్రం_శీర్షం* శిఖరం)
        : Reorder_అంచు{graphp, fromp, శిఖరం, CUTABLE} {}
    ~Reorder_Scorebd_అంచు() override = అప్రమేయ;
    తార్కిక followScoreboard() స్థిరరాశి override { తిరిగి నిజం; }
    string dotColor() స్థిరరాశి override { తిరిగి "blue"; }
};

తరగతి Reorder_Strict_అంచు final : బహిరంగ Reorder_అంచు {
    VL_RTTI_IMPL(Reorder_Strict_అంచు, Reorder_అంచు)
    // A strict order, based on the original statement order in the graph
    // The only non-cutable edge type
బహిరంగ:
    Reorder_Strict_అంచు(V3_లేఖాచిత్రం* graphp, V3_లేఖాచిత్రం_శీర్షం* fromp, V3_లేఖాచిత్రం_శీర్షం* శిఖరం)
        : Reorder_అంచు{graphp, fromp, శిఖరం, NOT_CUTABLE} {}
    ~Reorder_Strict_అంచు() override = అప్రమేయ;
    తార్కిక followScoreboard() స్థిరరాశి override { తిరిగి నిజం; }
    string dotColor() స్థిరరాశి override { తిరిగి "blue"; }
};

//######################################################################
// Reorder class functions

తరగతి ReorderVisitor final : బహిరంగ VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> ReorderVarStdVertex*.  Usage var, 0=not set yet
    // AstVarScope::user2p  -> ReorderVarPostVertex*.  Delayed assignment var, 0=not set yet
    // Ast*::user3p         -> ReorderLogicVertex*.  Statement (temporary only)
    // Ast*::user4          -> int.  Current ordering number (reorderBlock usage)

    // STATE
    V3_లేఖాచిత్రం* m_graphp = శూన్యసూచిక;  // Scoreboard of var usages/dependencies
    Reorder_Impure_శీర్షం* m_impureVtxp = శూన్యసూచిక;  // Element specifying PLI ordering
    తార్కిక m_లోపల_Dly = అబద్ధం;  // Inside ASSIGNDLY
    స్థిరరాశి అక్షరం* m_noReorderWhy = శూన్యసూచిక;  // Reason we can't reorder
    std::సదిశ<Reorder_Logic_శీర్షం*> m_stmtStackps;  // Current statements being tracked

    // METHODS
    శూన్య cleanup_ఖండం_లేఖాచిత్రం(Ast_కణుపు* nodep) {
        // Transform the graph into what we need
        UINFO(5, "ReorderBlock " << nodep);

        // Simplify graph by removing redundant edges
        m_graphp->remove_Redundant_అంచులు_Max(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_nodup", అబద్ధం);

        // Mark all the logic for this step by setting Vertex::user() to true
        m_graphp->userClearVertices();
        కొరకు (Ast_కణుపు* nextp = nodep; nextp; nextp = nextp->nextp()) {
            nextp->user3u().గమ్యం<Reorder_Logic_శీర్షం*>()->user(నిజం);
        }

        // New step
        Reorder_అంచు::increment_అడుగు();

        // If a var vertex has only inputs, it's a input-only node,
        // and can be ignored for coloring **this block only**
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_graphp->vertices()) {
            ఒకవేళ (!vtx.బహిర్గతం_ఖాళీలు()) కొనసాగు;
            ఒకవేళ (!vtx.is<Reorder_Var_Std_శీర్షం>()) కొనసాగు;
            కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vtx.లోపల_అంచులు()) అంచు.as<Reorder_అంచు>()->సమితి_Ignore_This_అడుగు();
        }

        // For reordering this single block only, mark all logic
        // vertexes not involved with this step as unimportant
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& శీర్షం : m_graphp->vertices()) {
            ఒకవేళ (శీర్షం.user()) కొనసాగు;
            ఒకవేళ (!శీర్షం.is<Reorder_Logic_శీర్షం>()) కొనసాగు;
            కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : శీర్షం.లోపల_అంచులు()) అంచు.as<Reorder_అంచు>()->సమితి_Ignore_This_అడుగు();
            కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : శీర్షం.బహిర్గతం_అంచులు())
                అంచు.as<Reorder_అంచు>()->సమితి_Ignore_This_అడుగు();
        }

        // Weak coloring to determine what needs to remain in order
        // This follows all step-relevant edges excluding PostEdges, which are done later
        m_graphp->weaklyConnected(&Reorder_అంచు::followScoreboard);

        // Add hard orderings between all nodes of same color, in the order they appeared
        std::unordered_map<uint32_t, Reorder_Logic_శీర్షం*> చివరి_యొక్క_Color;
        కొరకు (Ast_కణుపు* currp = nodep; currp; currp = currp->nextp()) {
            Reorder_Logic_శీర్షం* స్థిరరాశి vtxp = currp->user3u().గమ్యం<Reorder_Logic_శీర్షం*>();
            స్థిరరాశి uint32_t color = vtxp->color();
            ఒకవేళ (చివరి_యొక్క_Color[color]) కొత్త Reorder_Strict_అంచు{m_graphp, చివరి_యొక్క_Color[color], vtxp};
            చివరి_యొక్క_Color[color] = vtxp;
        }

        // And a real ordering to get the statements into something reasonable
        // We don't care if there's cutable violations here...
        // Non-cutable violations should be impossible; as those edges are program-order
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_pre", అబద్ధం);
        m_graphp->acyclic(&Reorder_అంచు::followCyclic);
        m_graphp->rank(&Reorder_అంచు::followCyclic);  // Or order(), but that's more expensive
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_opt", అబద్ధం);
    }

    శూన్య reorder_ఖండం(Ast_కణుపు* nodep) {
        // Reorder statements in the completed graph

        // Map the rank numbers into nodes they associate with
        std::multimap<uint32_t, Ast_కణుపు*> rankMap;
        పూర్ణాంకం curr_క్రమం = 0;  // Existing sequence number of assignment
        కొరకు (Ast_కణుపు* currp = nodep; currp; currp = currp->nextp()) {
            స్థిరరాశి Reorder_Logic_శీర్షం* స్థిరరాశి vtxp = currp->user3u().గమ్యం<Reorder_Logic_శీర్షం*>();
            rankMap.emplace(vtxp->rank(), currp);
            currp->user4(++curr_క్రమం);  // Record current ordering
        }

        // Is the current ordering OK?
        తార్కిక leaveAlone = నిజం;
        uint64_t new_క్రమం = 0;  // New sequence number of assignment
        కొరకు (స్థిరరాశి స్వయంచాలక& అంశం_2 : rankMap) {
            స్థిరరాశి Ast_కణుపు* స్థిరరాశి nextp = అంశం_2.రెండవ;
            ఒకవేళ (++new_క్రమం != nextp->user4()) leaveAlone = అబద్ధం;
        }
        ఒకవేళ (leaveAlone) {
            UINFO(6, "   No changes");
            తిరిగి;
        }

        VNRelinker replaceHandle;  // Where to add the list
        Ast_కణుపు* newListp = శూన్యసూచిక;
        కొరకు (స్థిరరాశి స్వయంచాలక& అంశం_2 : rankMap) {
            Ast_కణుపు* స్థిరరాశి nextp = అంశం_2.రెండవ;
            UINFO(6, "   New order: " << nextp);
            nextp->unlink_Fr_వెనుక(nextp == nodep ? &replaceHandle : శూన్యసూచిక);
            newListp = Ast_కణుపు::కూడు_తదుపరి(newListp, nextp);
        }
        replaceHandle.relink(newListp);
    }

    శూన్య process_ఖండం(Ast_కణుపు* nodep) {
        ఒకవేళ (m_noReorderWhy) తిరిగి;

        // Empty lists are ignorable
        ఒకవేళ (!nodep) తిరిగి;
        UASSERT_OBJ(nodep->మొదటి_Abovep(), nodep, "Node passed is in not head of list");
        UASSERT_OBJ(!nodep->user3p(), nodep, "Should not have a logic vertex");

        // It nothing to reorder with, just iterate
        ఒకవేళ (!nodep->nextp()) {
            iterate(nodep);
            తిరిగి;
        }

        // Process it
        UINFO(9, "  processBlock " << nodep);

        // Iterate across current block, making the scoreboard
        కొరకు (Ast_కణుపు* currp = nodep; currp; currp = currp->nextp()) {
            // Create the logic vertex for this statement
            UASSERT_OBJ(!currp->user3p(), currp, "user3p should not be set");
            Reorder_Logic_శీర్షం* స్థిరరాశి vtxp = కొత్త Reorder_Logic_శీర్షం{m_graphp, currp};
            currp->user3p(vtxp);

            // Visit the statement - this can recursively reorder sub statements
            m_stmtStackps.నెట్టు_వెనుక(vtxp);
            iterate(currp);
            m_stmtStackps.తీయి_వెనుక();
        }

        ఒకవేళ (m_noReorderWhy) {  // Jump or something nasty
            UINFO(9, "  NoReorderBlock because " << m_noReorderWhy);
            తిరిగి;
        }

        // Reorder statements in this block
        cleanup_ఖండం_లేఖాచిత్రం(nodep);
        reorder_ఖండం(nodep);

        // 'nodep' might no longer be the head of the list, rewind
        అయితే (nodep->backp()->nextp() == nodep) nodep = nodep->backp();

        // Delete vertexes and edges only applying to this block
        కొరకు (Ast_కణుపు* currp = nodep; currp; currp = currp->nextp()) {
            currp->user3u().గమ్యం<Reorder_Logic_శీర్షం*>()->unlinkDelete(m_graphp);
            currp->user3p(శూన్యసూచిక);
        }
    }

    // VISITORS
    శూన్య సందర్శించు(AstAlways* nodep) override {
        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_లోపల_Dly);
        VL_RESTORER(m_noReorderWhy);

        V3_లేఖాచిత్రం లేఖాచిత్రం;
        m_graphp = &లేఖాచిత్రం;
        m_impureVtxp = శూన్యసూచిక;
        m_లోపల_Dly = అబద్ధం;
        m_noReorderWhy = శూన్యసూచిక;

        స్థిరరాశి VNUser1_లోపల_Use user1_లోపల_Use;
        స్థిరరాశి VNUser2_లోపల_Use user2_లోపల_Use;
        స్థిరరాశి VNUser3_లోపల_Use user3_లోపల_Use;
        స్థిరరాశి VNUser4_లోపల_Use user4_లోపల_Use;

        UINFOTREE(9, nodep, "", "alwIn:");
        process_ఖండం(nodep->stmtsp());
        UINFOTREE(9, nodep, "", "alwOut");

        m_stmtStackps.clear();
    }

    శూన్య సందర్శించు(Ast_కణుపు_If* nodep) override {
        ఒకవేళ (!m_graphp || m_noReorderWhy) తిరిగి;
        iterate_And_తదుపరి_Null(nodep->condp());
        process_ఖండం(nodep->thensp());
        process_ఖండం(nodep->elsesp());
    }

    శూన్య సందర్శించు(AstJumpGo* nodep) override {
        ఒకవేళ (!m_graphp || m_noReorderWhy) తిరిగి;
        m_noReorderWhy = "JumpGo";
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు_CCall* nodep) override {
        ఒకవేళ (!m_graphp || m_noReorderWhy) తిరిగి;
        // The callee is not inlined, so the variables it reads and writes carry no VarRef at
        // the call site, and the scoreboard would let assignments to them move across the
        // call. Treat any such call as a barrier.
        m_noReorderWhy = "CCall";
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(Ast_సమాసం_Stmt* nodep) override {
        ఒకవేళ (!m_graphp || m_noReorderWhy) తిరిగి;
        VL_RESTORER(m_లోపల_Dly);
        m_లోపల_Dly = అబద్ధం;
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(Ast_కేటాయించు_Dly* nodep) override {
        ఒకవేళ (!m_graphp || m_noReorderWhy) తిరిగి;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_లోపల_Dly);
        m_లోపల_Dly = నిజం;
        iterate(nodep->lhsp());
    }

    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        ఒకవేళ (m_graphp) m_noReorderWhy = "Handle";
        సందర్శించు(static_cast<Ast_కణుపు*>(nodep));
    }

    శూన్య సందర్శించు(AstVarRef* nodep) override {
        ఒకవేళ (!m_graphp || m_noReorderWhy) తిరిగి;
        ఒకవేళ (m_stmtStackps.ఖాళీలు()) తిరిగి;

        // Reads of constants can be ignored - TODO: This should be "constexpr", not run-time const
        ఒకవేళ (nodep->varp()->isConst()) తిరిగి;

        // SPEEDUP: We add duplicate edges, that should be fixed

        AstVarScope* స్థిరరాశి vscp = nodep->varScopep();

        // Create vertexes for variable
        ఒకవేళ (!vscp->user1p()) vscp->user1p(కొత్త Reorder_Var_Std_శీర్షం{m_graphp, vscp});
        Reorder_Var_Std_శీర్షం* స్థిరరాశి vstdp = vscp->user1u().గమ్యం<Reorder_Var_Std_శీర్షం*>();

        // Variable is read
        ఒకవేళ (nodep->access().isReadOnly()) {
            కొరకు (Reorder_Logic_శీర్షం* స్థిరరాశి vtxp : m_stmtStackps) {
                కొత్త ReorderRVEdge{m_graphp, vtxp, vstdp};
            }
            తిరిగి;
        }

        // Variable is written, not NBA
        ఒకవేళ (!m_లోపల_Dly) {
            కొరకు (Reorder_Logic_శీర్షం* స్థిరరాశి vtxp : m_stmtStackps) {
                కొత్త ReorderLVEdge{m_graphp, vstdp, vtxp};
            }
            తిరిగి;
        }

        // Variable is written by NBA
        ఒకవేళ (!vscp->user2p()) {
            Reorder_Var_Post_శీర్షం* స్థిరరాశి vpostp = కొత్త Reorder_Var_Post_శీర్షం{m_graphp, vscp};
            vscp->user2p(vpostp);
            కొత్త Reorder_Post_అంచు{m_graphp, vstdp, vpostp};
        }
        Reorder_Var_Post_శీర్షం* స్థిరరాశి vpostp = vscp->user2u().గమ్యం<Reorder_Var_Post_శీర్షం*>();
        కొరకు (Reorder_Logic_శీర్షం* స్థిరరాశి vtxp : m_stmtStackps) {
            కొత్త ReorderLVEdge{m_graphp, vpostp, vtxp};
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        // Outside AstAlways, just descend
        ఒకవేళ (!m_graphp) {
            iterateChildren(nodep);
            తిరిగి;
        }

        // Early exit if decided not to reorder
        ఒకవేళ (m_noReorderWhy) తిరిగి;

        // Timing control prevents reordering
        ఒకవేళ (nodep->isTimingControl()) {
            m_noReorderWhy = "TimingControl";
            తిరిగి;
        }

        // Order all impure statements with other impure statements
        ఒకవేళ (!nodep->is_శుద్ధ()) {
            ఒకవేళ (!m_impureVtxp) m_impureVtxp = కొత్త Reorder_Impure_శీర్షం{m_graphp, nodep};
            // This edge is only used to find weakly connected components, so one edge is enough
            కొరకు (Reorder_Logic_శీర్షం* స్థిరరాశి vtxp : m_stmtStackps) {
                కొత్త Reorder_Scorebd_అంచు{m_graphp, m_impureVtxp, vtxp};
            }
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
బహిరంగ:
    స్పష్టమైన ReorderVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReorderVisitor() override = అప్రమేయ;
};

}  // namespace

//######################################################################
// V3Reorder class functions

శూన్య V3Reorder::reorderAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReorderVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("reorder", 0, dump_వృక్షం_Either_Level() >= 3);
}
