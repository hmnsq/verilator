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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Reorder.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitV.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <string>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ਨਾਮਾਕਾਸ਼ {

//######################################################################
// Support classes

ਵਰਗ Reorder_ਗੰਢ_ਸ਼ੀਰਸ਼ VL_NOT_FINAL : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Reorder_ਗੰਢ_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
    Ast_ਗੰਢ* ਅਚਲ m_nodep;

ਸੁਰੱਖਿਅਤ:
    Reorder_ਗੰਢ_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, Ast_ਗੰਢ* nodep)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp}
        , m_nodep{nodep} {}
    ~Reorder_ਗੰਢ_ਸ਼ੀਰਸ਼() override = ਮੂਲ;
    // ACCESSORS
    // Do not make accessor for nodep(),  It may change due to
    // reordering a lower block, but we don't repair it
    std::string name() ਅਚਲ override {
        std::string str = cvt_ਮੰਜ਼ਿਲ_Hex(m_nodep) + '\n';
        ਜੇ (AstVarScope* ਅਚਲ vscp = VN_CAST(m_nodep, VarScope)) {
            str += vscp->prettyName();
        } ਨਹੀਂਤਾਂ {
            str += V3EmitV::debug_Verilog_For_ਰੁੱਖ(m_nodep);
            str = VString::quoteBackslash(str);
            str = VString::quoteAny(str, '"', '\\');
            str = VString::replaceSubstr(str, "\n", "\\l");
        }
        ਵਾਪਸ str;
    }
    File_ਸਤਰ* fileline() ਅਚਲ override { ਵਾਪਸ nodep()->fileline(); }
    std::string dotShape() ਅਚਲ override { ਵਾਪਸ VN_IS(m_nodep, VarScope) ? "ellipse" : "box"; }

ਜਨਤਕ:
    ਆਭਾਸੀ Ast_ਗੰਢ* nodep() ਅਚਲ { ਵਾਪਸ m_nodep; }
};

ਵਰਗ Reorder_Impure_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ Reorder_ਗੰਢ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Reorder_Impure_ਸ਼ੀਰਸ਼, Reorder_ਗੰਢ_ਸ਼ੀਰਸ਼)
ਜਨਤਕ:
    ਸਪਸ਼ਟ Reorder_Impure_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, Ast_ਗੰਢ* nodep)
        : Reorder_ਗੰਢ_ਸ਼ੀਰਸ਼{graphp, nodep} {}
    ~Reorder_Impure_ਸ਼ੀਰਸ਼() override = ਮੂਲ;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ "*IMPURE*"; }
    string dotColor() ਅਚਲ override { ਵਾਪਸ "red"; }
};

ਵਰਗ Reorder_Logic_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ Reorder_ਗੰਢ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Reorder_Logic_ਸ਼ੀਰਸ਼, Reorder_ਗੰਢ_ਸ਼ੀਰਸ਼)
ਜਨਤਕ:
    Reorder_Logic_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, Ast_ਗੰਢ* nodep)
        : Reorder_ਗੰਢ_ਸ਼ੀਰਸ਼{graphp, nodep} {}
    ~Reorder_Logic_ਸ਼ੀਰਸ਼() override = ਮੂਲ;
    string dotColor() ਅਚਲ override { ਵਾਪਸ "black"; }
};

ਵਰਗ Reorder_Var_Std_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ Reorder_ਗੰਢ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Reorder_Var_Std_ਸ਼ੀਰਸ਼, Reorder_ਗੰਢ_ਸ਼ੀਰਸ਼)
ਜਨਤਕ:
    Reorder_Var_Std_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, AstVarScope* nodep)
        : Reorder_ਗੰਢ_ਸ਼ੀਰਸ਼{graphp, nodep} {}
    ~Reorder_Var_Std_ਸ਼ੀਰਸ਼() override = ਮੂਲ;
    string dotColor() ਅਚਲ override { ਵਾਪਸ "blue"; }
};

ਵਰਗ Reorder_Var_Post_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ Reorder_ਗੰਢ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Reorder_Var_Post_ਸ਼ੀਰਸ਼, Reorder_ਗੰਢ_ਸ਼ੀਰਸ਼)
ਜਨਤਕ:
    Reorder_Var_Post_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, AstVarScope* nodep)
        : Reorder_ਗੰਢ_ਸ਼ੀਰਸ਼{graphp, nodep} {}
    ~Reorder_Var_Post_ਸ਼ੀਰਸ਼() override = ਮੂਲ;
    string name() ਅਚਲ override { ਵਾਪਸ "POST "s + Reorder_ਗੰਢ_ਸ਼ੀਰਸ਼::name(); }
    string dotColor() ਅਚਲ override { ਵਾਪਸ "green"; }
};

//######################################################################
// Edge types

ਵਰਗ Reorder_ਕਿਨਾਰਾ VL_NOT_FINAL : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ {
    VL_RTTI_IMPL(Reorder_ਕਿਨਾਰਾ, V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ)
    uint32_t m_ignore_ਵਿੱਚ_ਕਦਮ = 0;  // Step number that if set to, causes this edge to be ignored
    ਸਥਿਰ uint32_t s_ਕਦਮ_ਅੰਕ;  // Global step number
ਸੁਰੱਖਿਅਤ:
    ਸਥਿਰ constexpr ਪੂਰਨ ਭਾਰ_ਸਾਧਾਰਨ = 10;
    Reorder_ਕਿਨਾਰਾ(V3_ਗ੍ਰਾਫ* graphp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ, ਤਾਰਕਿਕ cutable)
        : V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{graphp, fromp, ਸਿਖਰ, ਭਾਰ_ਸਾਧਾਰਨ, cutable} {}
    ~Reorder_ਕਿਨਾਰਾ() override = ਮੂਲ;

    ਆਭਾਸੀ ਤਾਰਕਿਕ followScoreboard() ਅਚਲ = 0;

    std::string dotStyle() ਅਚਲ override {
        ਵਾਪਸ ignore_This_ਕਦਮ() ? "dotted" : V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::dotStyle();
    }

ਜਨਤਕ:
    // Iterator for graph functions
    ਸਥਿਰ ਖਾਲੀ increment_ਕਦਮ() { ++s_ਕਦਮ_ਅੰਕ; }
    ਤਾਰਕਿਕ ignore_This_ਕਦਮ() ਅਚਲ { ਵਾਪਸ m_ignore_ਵਿੱਚ_ਕਦਮ == s_ਕਦਮ_ਅੰਕ; }
    ਖਾਲੀ ਸੈੱਟ_Ignore_This_ਕਦਮ() { m_ignore_ਵਿੱਚ_ਕਦਮ = s_ਕਦਮ_ਅੰਕ; }

    ਸਥਿਰ ਤਾਰਕਿਕ followScoreboard(ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* edgep) {
        ਅਚਲ Reorder_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ = *edgep->as<Reorder_ਕਿਨਾਰਾ>();
        ਵਾਪਸ !ਕਿਨਾਰਾ.ignore_This_ਕਦਮ() && ਕਿਨਾਰਾ.followScoreboard();
    }
    ਸਥਿਰ ਤਾਰਕਿਕ followCyclic(ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* edgep) {
        ਅਚਲ Reorder_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ = *edgep->as<Reorder_ਕਿਨਾਰਾ>();
        ਵਾਪਸ !ਕਿਨਾਰਾ.ignore_This_ਕਦਮ();
    }
};
uint32_t Reorder_ਕਿਨਾਰਾ::s_ਕਦਮ_ਅੰਕ = 0;

ਵਰਗ Reorder_Post_ਕਿਨਾਰਾ final : ਜਨਤਕ Reorder_ਕਿਨਾਰਾ {
    VL_RTTI_IMPL(Reorder_Post_ਕਿਨਾਰਾ, Reorder_ਕਿਨਾਰਾ)
ਜਨਤਕ:
    Reorder_Post_ਕਿਨਾਰਾ(V3_ਗ੍ਰਾਫ* graphp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ)
        : Reorder_ਕਿਨਾਰਾ{graphp, fromp, ਸਿਖਰ, CUTABLE} {}
    ~Reorder_Post_ਕਿਨਾਰਾ() override = ਮੂਲ;
    ਤਾਰਕਿਕ followScoreboard() ਅਚਲ override { ਵਾਪਸ ਝੂਠ; }
    string dotColor() ਅਚਲ override { ਵਾਪਸ "khaki"; }
};

ਵਰਗ ReorderLVEdge final : ਜਨਤਕ Reorder_ਕਿਨਾਰਾ {
    VL_RTTI_IMPL(ReorderLVEdge, Reorder_ਕਿਨਾਰਾ)
ਜਨਤਕ:
    ReorderLVEdge(V3_ਗ੍ਰਾਫ* graphp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ)
        : Reorder_ਕਿਨਾਰਾ{graphp, fromp, ਸਿਖਰ, CUTABLE} {}
    ~ReorderLVEdge() override = ਮੂਲ;
    ਤਾਰਕਿਕ followScoreboard() ਅਚਲ override { ਵਾਪਸ ਸੱਚ; }
    string dotColor() ਅਚਲ override { ਵਾਪਸ "yellowGreen"; }
};

ਵਰਗ ReorderRVEdge final : ਜਨਤਕ Reorder_ਕਿਨਾਰਾ {
    VL_RTTI_IMPL(ReorderRVEdge, Reorder_ਕਿਨਾਰਾ)
ਜਨਤਕ:
    ReorderRVEdge(V3_ਗ੍ਰਾਫ* graphp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ)
        : Reorder_ਕਿਨਾਰਾ{graphp, fromp, ਸਿਖਰ, CUTABLE} {}
    ~ReorderRVEdge() override = ਮੂਲ;
    ਤਾਰਕਿਕ followScoreboard() ਅਚਲ override { ਵਾਪਸ ਸੱਚ; }
    string dotColor() ਅਚਲ override { ਵਾਪਸ "green"; }
};

ਵਰਗ Reorder_Scorebd_ਕਿਨਾਰਾ final : ਜਨਤਕ Reorder_ਕਿਨਾਰਾ {
    VL_RTTI_IMPL(Reorder_Scorebd_ਕਿਨਾਰਾ, Reorder_ਕਿਨਾਰਾ)
ਜਨਤਕ:
    Reorder_Scorebd_ਕਿਨਾਰਾ(V3_ਗ੍ਰਾਫ* graphp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ)
        : Reorder_ਕਿਨਾਰਾ{graphp, fromp, ਸਿਖਰ, CUTABLE} {}
    ~Reorder_Scorebd_ਕਿਨਾਰਾ() override = ਮੂਲ;
    ਤਾਰਕਿਕ followScoreboard() ਅਚਲ override { ਵਾਪਸ ਸੱਚ; }
    string dotColor() ਅਚਲ override { ਵਾਪਸ "blue"; }
};

ਵਰਗ Reorder_Strict_ਕਿਨਾਰਾ final : ਜਨਤਕ Reorder_ਕਿਨਾਰਾ {
    VL_RTTI_IMPL(Reorder_Strict_ਕਿਨਾਰਾ, Reorder_ਕਿਨਾਰਾ)
    // A strict order, based on the original statement order in the graph
    // The only non-cutable edge type
ਜਨਤਕ:
    Reorder_Strict_ਕਿਨਾਰਾ(V3_ਗ੍ਰਾਫ* graphp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ)
        : Reorder_ਕਿਨਾਰਾ{graphp, fromp, ਸਿਖਰ, NOT_CUTABLE} {}
    ~Reorder_Strict_ਕਿਨਾਰਾ() override = ਮੂਲ;
    ਤਾਰਕਿਕ followScoreboard() ਅਚਲ override { ਵਾਪਸ ਸੱਚ; }
    string dotColor() ਅਚਲ override { ਵਾਪਸ "blue"; }
};

//######################################################################
// Reorder class functions

ਵਰਗ ReorderVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> ReorderVarStdVertex*.  Usage var, 0=not set yet
    // AstVarScope::user2p  -> ReorderVarPostVertex*.  Delayed assignment var, 0=not set yet
    // Ast*::user3p         -> ReorderLogicVertex*.  Statement (temporary only)
    // Ast*::user4          -> int.  Current ordering number (reorderBlock usage)

    // STATE
    V3_ਗ੍ਰਾਫ* m_graphp = ਖਾਲੀ_ਸੂਚਕ;  // Scoreboard of var usages/dependencies
    Reorder_Impure_ਸ਼ੀਰਸ਼* m_impureVtxp = ਖਾਲੀ_ਸੂਚਕ;  // Element specifying PLI ordering
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Dly = ਝੂਠ;  // Inside ASSIGNDLY
    ਅਚਲ ਅੱਖਰ* m_noReorderWhy = ਖਾਲੀ_ਸੂਚਕ;  // Reason we can't reorder
    std::ਸਦਿਸ਼<Reorder_Logic_ਸ਼ੀਰਸ਼*> m_stmtStackps;  // Current statements being tracked

    // METHODS
    ਖਾਲੀ cleanup_ਖੰਡ_ਗ੍ਰਾਫ(Ast_ਗੰਢ* nodep) {
        // Transform the graph into what we need
        UINFO(5, "ReorderBlock " << nodep);

        // Simplify graph by removing redundant edges
        m_graphp->remove_Redundant_ਕਿਨਾਰੇ_Max(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_nodup", ਝੂਠ);

        // Mark all the logic for this step by setting Vertex::user() to true
        m_graphp->userClearVertices();
        ਲਈ (Ast_ਗੰਢ* nextp = nodep; nextp; nextp = nextp->nextp()) {
            nextp->user3u().ਮੰਜ਼ਿਲ<Reorder_Logic_ਸ਼ੀਰਸ਼*>()->user(ਸੱਚ);
        }

        // New step
        Reorder_ਕਿਨਾਰਾ::increment_ਕਦਮ();

        // If a var vertex has only inputs, it's a input-only node,
        // and can be ignored for coloring **this block only**
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_graphp->vertices()) {
            ਜੇ (!vtx.ਬਾਹਰਲਾ_ਖਾਲੀ_ਥਾਵਾਂ()) ਜਾਰੀ;
            ਜੇ (!vtx.is<Reorder_Var_Std_ਸ਼ੀਰਸ਼>()) ਜਾਰੀ;
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtx.ਵਿੱਚ_ਕਿਨਾਰੇ()) ਕਿਨਾਰਾ.as<Reorder_ਕਿਨਾਰਾ>()->ਸੈੱਟ_Ignore_This_ਕਦਮ();
        }

        // For reordering this single block only, mark all logic
        // vertexes not involved with this step as unimportant
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : m_graphp->vertices()) {
            ਜੇ (ਸ਼ੀਰਸ਼.user()) ਜਾਰੀ;
            ਜੇ (!ਸ਼ੀਰਸ਼.is<Reorder_Logic_ਸ਼ੀਰਸ਼>()) ਜਾਰੀ;
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : ਸ਼ੀਰਸ਼.ਵਿੱਚ_ਕਿਨਾਰੇ()) ਕਿਨਾਰਾ.as<Reorder_ਕਿਨਾਰਾ>()->ਸੈੱਟ_Ignore_This_ਕਦਮ();
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : ਸ਼ੀਰਸ਼.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ())
                ਕਿਨਾਰਾ.as<Reorder_ਕਿਨਾਰਾ>()->ਸੈੱਟ_Ignore_This_ਕਦਮ();
        }

        // Weak coloring to determine what needs to remain in order
        // This follows all step-relevant edges excluding PostEdges, which are done later
        m_graphp->weaklyConnected(&Reorder_ਕਿਨਾਰਾ::followScoreboard);

        // Add hard orderings between all nodes of same color, in the order they appeared
        std::unordered_map<uint32_t, Reorder_Logic_ਸ਼ੀਰਸ਼*> ਆਖਰੀ_ਦਾ_Color;
        ਲਈ (Ast_ਗੰਢ* currp = nodep; currp; currp = currp->nextp()) {
            Reorder_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = currp->user3u().ਮੰਜ਼ਿਲ<Reorder_Logic_ਸ਼ੀਰਸ਼*>();
            ਅਚਲ uint32_t color = vtxp->color();
            ਜੇ (ਆਖਰੀ_ਦਾ_Color[color]) ਨਵਾਂ Reorder_Strict_ਕਿਨਾਰਾ{m_graphp, ਆਖਰੀ_ਦਾ_Color[color], vtxp};
            ਆਖਰੀ_ਦਾ_Color[color] = vtxp;
        }

        // And a real ordering to get the statements into something reasonable
        // We don't care if there's cutable violations here...
        // Non-cutable violations should be impossible; as those edges are program-order
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_pre", ਝੂਠ);
        m_graphp->acyclic(&Reorder_ਕਿਨਾਰਾ::followCyclic);
        m_graphp->rank(&Reorder_ਕਿਨਾਰਾ::followCyclic);  // Or order(), but that's more expensive
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_opt", ਝੂਠ);
    }

    ਖਾਲੀ reorder_ਖੰਡ(Ast_ਗੰਢ* nodep) {
        // Reorder statements in the completed graph

        // Map the rank numbers into nodes they associate with
        std::multimap<uint32_t, Ast_ਗੰਢ*> rankMap;
        ਪੂਰਨ curr_ਕ੍ਰਮ = 0;  // Existing sequence number of assignment
        ਲਈ (Ast_ਗੰਢ* currp = nodep; currp; currp = currp->nextp()) {
            ਅਚਲ Reorder_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = currp->user3u().ਮੰਜ਼ਿਲ<Reorder_Logic_ਸ਼ੀਰਸ਼*>();
            rankMap.emplace(vtxp->rank(), currp);
            currp->user4(++curr_ਕ੍ਰਮ);  // Record current ordering
        }

        // Is the current ordering OK?
        ਤਾਰਕਿਕ leaveAlone = ਸੱਚ;
        uint64_t new_ਕ੍ਰਮ = 0;  // New sequence number of assignment
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਵਸਤੂ_3 : rankMap) {
            ਅਚਲ Ast_ਗੰਢ* ਅਚਲ nextp = ਵਸਤੂ_3.ਦੂਜਾ;
            ਜੇ (++new_ਕ੍ਰਮ != nextp->user4()) leaveAlone = ਝੂਠ;
        }
        ਜੇ (leaveAlone) {
            UINFO(6, "   No changes");
            ਵਾਪਸ;
        }

        VNRelinker replaceHandle;  // Where to add the list
        Ast_ਗੰਢ* newListp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਵਸਤੂ_3 : rankMap) {
            Ast_ਗੰਢ* ਅਚਲ nextp = ਵਸਤੂ_3.ਦੂਜਾ;
            UINFO(6, "   New order: " << nextp);
            nextp->unlink_Fr_ਪਿੱਠ(nextp == nodep ? &replaceHandle : ਖਾਲੀ_ਸੂਚਕ);
            newListp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(newListp, nextp);
        }
        replaceHandle.relink(newListp);
    }

    ਖਾਲੀ process_ਖੰਡ(Ast_ਗੰਢ* nodep) {
        ਜੇ (m_noReorderWhy) ਵਾਪਸ;

        // Empty lists are ignorable
        ਜੇ (!nodep) ਵਾਪਸ;
        UASSERT_OBJ(nodep->ਪਹਿਲਾ_Abovep(), nodep, "Node passed is in not head of list");
        UASSERT_OBJ(!nodep->user3p(), nodep, "Should not have a logic vertex");

        // It nothing to reorder with, just iterate
        ਜੇ (!nodep->nextp()) {
            iterate(nodep);
            ਵਾਪਸ;
        }

        // Process it
        UINFO(9, "  processBlock " << nodep);

        // Iterate across current block, making the scoreboard
        ਲਈ (Ast_ਗੰਢ* currp = nodep; currp; currp = currp->nextp()) {
            // Create the logic vertex for this statement
            UASSERT_OBJ(!currp->user3p(), currp, "user3p should not be set");
            Reorder_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = ਨਵਾਂ Reorder_Logic_ਸ਼ੀਰਸ਼{m_graphp, currp};
            currp->user3p(vtxp);

            // Visit the statement - this can recursively reorder sub statements
            m_stmtStackps.ਧੱਕੋ_ਪਿੱਠ(vtxp);
            iterate(currp);
            m_stmtStackps.ਕੱਢੋ_ਪਿੱਠ();
        }

        ਜੇ (m_noReorderWhy) {  // Jump or something nasty
            UINFO(9, "  NoReorderBlock because " << m_noReorderWhy);
            ਵਾਪਸ;
        }

        // Reorder statements in this block
        cleanup_ਖੰਡ_ਗ੍ਰਾਫ(nodep);
        reorder_ਖੰਡ(nodep);

        // 'nodep' might no longer be the head of the list, rewind
        ਜਦੋਂ (nodep->backp()->nextp() == nodep) nodep = nodep->backp();

        // Delete vertexes and edges only applying to this block
        ਲਈ (Ast_ਗੰਢ* currp = nodep; currp; currp = currp->nextp()) {
            currp->user3u().ਮੰਜ਼ਿਲ<Reorder_Logic_ਸ਼ੀਰਸ਼*>()->unlinkDelete(m_graphp);
            currp->user3p(ਖਾਲੀ_ਸੂਚਕ);
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstAlways* nodep) override {
        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_ਵਿੱਚ_Dly);
        VL_RESTORER(m_noReorderWhy);

        V3_ਗ੍ਰਾਫ ਗ੍ਰਾਫ;
        m_graphp = &ਗ੍ਰਾਫ;
        m_impureVtxp = ਖਾਲੀ_ਸੂਚਕ;
        m_ਵਿੱਚ_Dly = ਝੂਠ;
        m_noReorderWhy = ਖਾਲੀ_ਸੂਚਕ;

        ਅਚਲ VNUser1_ਵਿੱਚ_Use user1_ਵਿੱਚ_Use;
        ਅਚਲ VNUser2_ਵਿੱਚ_Use user2_ਵਿੱਚ_Use;
        ਅਚਲ VNUser3_ਵਿੱਚ_Use user3_ਵਿੱਚ_Use;
        ਅਚਲ VNUser4_ਵਿੱਚ_Use user4_ਵਿੱਚ_Use;

        UINFOTREE(9, nodep, "", "alwIn:");
        process_ਖੰਡ(nodep->stmtsp());
        UINFOTREE(9, nodep, "", "alwOut");

        m_stmtStackps.clear();
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_If* nodep) override {
        ਜੇ (!m_graphp || m_noReorderWhy) ਵਾਪਸ;
        iterate_And_ਅਗਲਾ_Null(nodep->condp());
        process_ਖੰਡ(nodep->thensp());
        process_ਖੰਡ(nodep->elsesp());
    }

    ਖਾਲੀ ਦੇਖੋ(AstJumpGo* nodep) override {
        ਜੇ (!m_graphp || m_noReorderWhy) ਵਾਪਸ;
        m_noReorderWhy = "JumpGo";
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_CCall* nodep) override {
        ਜੇ (!m_graphp || m_noReorderWhy) ਵਾਪਸ;
        // The callee is not inlined, so the variables it reads and writes carry no VarRef at
        // the call site, and the scoreboard would let assignments to them move across the
        // call. Treat any such call as a barrier.
        m_noReorderWhy = "CCall";
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਵਿਅੰਜਕ_Stmt* nodep) override {
        ਜੇ (!m_graphp || m_noReorderWhy) ਵਾਪਸ;
        VL_RESTORER(m_ਵਿੱਚ_Dly);
        m_ਵਿੱਚ_Dly = ਝੂਠ;
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Dly* nodep) override {
        ਜੇ (!m_graphp || m_noReorderWhy) ਵਾਪਸ;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_ਵਿੱਚ_Dly);
        m_ਵਿੱਚ_Dly = ਸੱਚ;
        iterate(nodep->lhsp());
    }

    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        ਜੇ (m_graphp) m_noReorderWhy = "Handle";
        ਦੇਖੋ(static_cast<Ast_ਗੰਢ*>(nodep));
    }

    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        ਜੇ (!m_graphp || m_noReorderWhy) ਵਾਪਸ;
        ਜੇ (m_stmtStackps.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;

        // Reads of constants can be ignored - TODO: This should be "constexpr", not run-time const
        ਜੇ (nodep->varp()->isConst()) ਵਾਪਸ;

        // SPEEDUP: We add duplicate edges, that should be fixed

        AstVarScope* ਅਚਲ vscp = nodep->varScopep();

        // Create vertexes for variable
        ਜੇ (!vscp->user1p()) vscp->user1p(ਨਵਾਂ Reorder_Var_Std_ਸ਼ੀਰਸ਼{m_graphp, vscp});
        Reorder_Var_Std_ਸ਼ੀਰਸ਼* ਅਚਲ vstdp = vscp->user1u().ਮੰਜ਼ਿਲ<Reorder_Var_Std_ਸ਼ੀਰਸ਼*>();

        // Variable is read
        ਜੇ (nodep->access().isReadOnly()) {
            ਲਈ (Reorder_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_stmtStackps) {
                ਨਵਾਂ ReorderRVEdge{m_graphp, vtxp, vstdp};
            }
            ਵਾਪਸ;
        }

        // Variable is written, not NBA
        ਜੇ (!m_ਵਿੱਚ_Dly) {
            ਲਈ (Reorder_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_stmtStackps) {
                ਨਵਾਂ ReorderLVEdge{m_graphp, vstdp, vtxp};
            }
            ਵਾਪਸ;
        }

        // Variable is written by NBA
        ਜੇ (!vscp->user2p()) {
            Reorder_Var_Post_ਸ਼ੀਰਸ਼* ਅਚਲ vpostp = ਨਵਾਂ Reorder_Var_Post_ਸ਼ੀਰਸ਼{m_graphp, vscp};
            vscp->user2p(vpostp);
            ਨਵਾਂ Reorder_Post_ਕਿਨਾਰਾ{m_graphp, vstdp, vpostp};
        }
        Reorder_Var_Post_ਸ਼ੀਰਸ਼* ਅਚਲ vpostp = vscp->user2u().ਮੰਜ਼ਿਲ<Reorder_Var_Post_ਸ਼ੀਰਸ਼*>();
        ਲਈ (Reorder_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_stmtStackps) {
            ਨਵਾਂ ReorderLVEdge{m_graphp, vpostp, vtxp};
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        // Outside AstAlways, just descend
        ਜੇ (!m_graphp) {
            iterateChildren(nodep);
            ਵਾਪਸ;
        }

        // Early exit if decided not to reorder
        ਜੇ (m_noReorderWhy) ਵਾਪਸ;

        // Timing control prevents reordering
        ਜੇ (nodep->isTimingControl()) {
            m_noReorderWhy = "TimingControl";
            ਵਾਪਸ;
        }

        // Order all impure statements with other impure statements
        ਜੇ (!nodep->is_ਸ਼ੁੱਧ()) {
            ਜੇ (!m_impureVtxp) m_impureVtxp = ਨਵਾਂ Reorder_Impure_ਸ਼ੀਰਸ਼{m_graphp, nodep};
            // This edge is only used to find weakly connected components, so one edge is enough
            ਲਈ (Reorder_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_stmtStackps) {
                ਨਵਾਂ Reorder_Scorebd_ਕਿਨਾਰਾ{m_graphp, m_impureVtxp, vtxp};
            }
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
ਜਨਤਕ:
    ਸਪਸ਼ਟ ReorderVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReorderVisitor() override = ਮੂਲ;
};

}  // namespace

//######################################################################
// V3Reorder class functions

ਖਾਲੀ V3Reorder::reorderAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReorderVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("reorder", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
