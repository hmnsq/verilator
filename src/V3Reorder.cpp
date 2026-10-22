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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Reorder.h"

#ಅನ್ತರ್ಭಾವ "V3EmitV.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <string>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ನಾಮಾಕಾಶ {

//######################################################################
// Support classes

ವರ್ಗ Reorder_ಗಂಟು_ಶೃಂಗ VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Reorder_ಗಂಟು_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ m_nodep;

ಸಂರಕ್ಷಿತ:
    Reorder_ಗಂಟು_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, Ast_ಗಂಟು* nodep)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp}
        , m_nodep{nodep} {}
    ~Reorder_ಗಂಟು_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    // ACCESSORS
    // Do not make accessor for nodep(),  It may change due to
    // reordering a lower block, but we don't repair it
    std::string name() ಸ್ಥಿರಾಂಕ override {
        std::string str = cvt_ಗಮ್ಯ_Hex(m_nodep) + '\n';
        ಆದರೆ (AstVarScope* ಸ್ಥಿರಾಂಕ vscp = VN_CAST(m_nodep, VarScope)) {
            str += vscp->prettyName();
        } ಇಲ್ಲದಿದ್ದರೆ {
            str += V3EmitV::debug_Verilog_For_ವೃಕ್ಷ(m_nodep);
            str = VString::quoteBackslash(str);
            str = VString::quoteAny(str, '"', '\\');
            str = VString::replaceSubstr(str, "\n", "\\l");
        }
        ಹಿಂತಿರುಗಿ str;
    }
    File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ nodep()->fileline(); }
    std::string dotShape() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ VN_IS(m_nodep, VarScope) ? "ellipse" : "box"; }

ಸಾರ್ವಜನಿಕ:
    ವಾಸ್ತವಿಕ Ast_ಗಂಟು* nodep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_nodep; }
};

ವರ್ಗ Reorder_Impure_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ Reorder_ಗಂಟು_ಶೃಂಗ {
    VL_RTTI_IMPL(Reorder_Impure_ಶೃಂಗ, Reorder_ಗಂಟು_ಶೃಂಗ)
ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ Reorder_Impure_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, Ast_ಗಂಟು* nodep)
        : Reorder_ಗಂಟು_ಶೃಂಗ{graphp, nodep} {}
    ~Reorder_Impure_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ "*IMPURE*"; }
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "red"; }
};

ವರ್ಗ Reorder_Logic_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ Reorder_ಗಂಟು_ಶೃಂಗ {
    VL_RTTI_IMPL(Reorder_Logic_ಶೃಂಗ, Reorder_ಗಂಟು_ಶೃಂಗ)
ಸಾರ್ವಜನಿಕ:
    Reorder_Logic_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, Ast_ಗಂಟು* nodep)
        : Reorder_ಗಂಟು_ಶೃಂಗ{graphp, nodep} {}
    ~Reorder_Logic_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "black"; }
};

ವರ್ಗ Reorder_Var_Std_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ Reorder_ಗಂಟು_ಶೃಂಗ {
    VL_RTTI_IMPL(Reorder_Var_Std_ಶೃಂಗ, Reorder_ಗಂಟು_ಶೃಂಗ)
ಸಾರ್ವಜನಿಕ:
    Reorder_Var_Std_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, AstVarScope* nodep)
        : Reorder_ಗಂಟು_ಶೃಂಗ{graphp, nodep} {}
    ~Reorder_Var_Std_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "blue"; }
};

ವರ್ಗ Reorder_Var_Post_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ Reorder_ಗಂಟು_ಶೃಂಗ {
    VL_RTTI_IMPL(Reorder_Var_Post_ಶೃಂಗ, Reorder_ಗಂಟು_ಶೃಂಗ)
ಸಾರ್ವಜನಿಕ:
    Reorder_Var_Post_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, AstVarScope* nodep)
        : Reorder_ಗಂಟು_ಶೃಂಗ{graphp, nodep} {}
    ~Reorder_Var_Post_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    string name() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "POST "s + Reorder_ಗಂಟು_ಶೃಂಗ::name(); }
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "green"; }
};

//######################################################################
// Edge types

ವರ್ಗ Reorder_ಅಂಚು VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಅಂಚು {
    VL_RTTI_IMPL(Reorder_ಅಂಚು, V3_ನಕ್ಷೆ_ಅಂಚು)
    uint32_t m_ignore_ಒಳ_ಹೆಜ್ಜೆ = 0;  // Step number that if set to, causes this edge to be ignored
    ಸ್ಥಿರ uint32_t s_ಹೆಜ್ಜೆ_ಅಂಕೆ;  // Global step number
ಸಂರಕ್ಷಿತ:
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ ತೂಕ_ಸಾಮಾನ್ಯ = 10;
    Reorder_ಅಂಚು(V3_ನಕ್ಷೆ* graphp, V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp, V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ, ತಾರ್ಕಿಕ cutable)
        : V3_ನಕ್ಷೆ_ಅಂಚು{graphp, fromp, ತುದಿ, ತೂಕ_ಸಾಮಾನ್ಯ, cutable} {}
    ~Reorder_ಅಂಚು() override = ಪೂರ್ವನಿಯೋಜಿತ;

    ವಾಸ್ತವಿಕ ತಾರ್ಕಿಕ followScoreboard() ಸ್ಥಿರಾಂಕ = 0;

    std::string dotStyle() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ ignore_This_ಹೆಜ್ಜೆ() ? "dotted" : V3_ನಕ್ಷೆ_ಅಂಚು::dotStyle();
    }

ಸಾರ್ವಜನಿಕ:
    // Iterator for graph functions
    ಸ್ಥಿರ ಶೂನ್ಯ increment_ಹೆಜ್ಜೆ() { ++s_ಹೆಜ್ಜೆ_ಅಂಕೆ; }
    ತಾರ್ಕಿಕ ignore_This_ಹೆಜ್ಜೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ignore_ಒಳ_ಹೆಜ್ಜೆ == s_ಹೆಜ್ಜೆ_ಅಂಕೆ; }
    ಶೂನ್ಯ ಗಣ_Ignore_This_ಹೆಜ್ಜೆ() { m_ignore_ಒಳ_ಹೆಜ್ಜೆ = s_ಹೆಜ್ಜೆ_ಅಂಕೆ; }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ followScoreboard(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು* edgep) {
        ಸ್ಥಿರಾಂಕ Reorder_ಅಂಚು& ಅಂಚು = *edgep->as<Reorder_ಅಂಚು>();
        ಹಿಂತಿರುಗಿ !ಅಂಚು.ignore_This_ಹೆಜ್ಜೆ() && ಅಂಚು.followScoreboard();
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ followCyclic(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು* edgep) {
        ಸ್ಥಿರಾಂಕ Reorder_ಅಂಚು& ಅಂಚು = *edgep->as<Reorder_ಅಂಚು>();
        ಹಿಂತಿರುಗಿ !ಅಂಚು.ignore_This_ಹೆಜ್ಜೆ();
    }
};
uint32_t Reorder_ಅಂಚು::s_ಹೆಜ್ಜೆ_ಅಂಕೆ = 0;

ವರ್ಗ Reorder_Post_ಅಂಚು final : ಸಾರ್ವಜನಿಕ Reorder_ಅಂಚು {
    VL_RTTI_IMPL(Reorder_Post_ಅಂಚು, Reorder_ಅಂಚು)
ಸಾರ್ವಜನಿಕ:
    Reorder_Post_ಅಂಚು(V3_ನಕ್ಷೆ* graphp, V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp, V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ)
        : Reorder_ಅಂಚು{graphp, fromp, ತುದಿ, CUTABLE} {}
    ~Reorder_Post_ಅಂಚು() override = ಪೂರ್ವನಿಯೋಜಿತ;
    ತಾರ್ಕಿಕ followScoreboard() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಸುಳ್ಳು; }
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "khaki"; }
};

ವರ್ಗ ReorderLVEdge final : ಸಾರ್ವಜನಿಕ Reorder_ಅಂಚು {
    VL_RTTI_IMPL(ReorderLVEdge, Reorder_ಅಂಚು)
ಸಾರ್ವಜನಿಕ:
    ReorderLVEdge(V3_ನಕ್ಷೆ* graphp, V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp, V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ)
        : Reorder_ಅಂಚು{graphp, fromp, ತುದಿ, CUTABLE} {}
    ~ReorderLVEdge() override = ಪೂರ್ವನಿಯೋಜಿತ;
    ತಾರ್ಕಿಕ followScoreboard() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ನಿಜ; }
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "yellowGreen"; }
};

ವರ್ಗ ReorderRVEdge final : ಸಾರ್ವಜನಿಕ Reorder_ಅಂಚು {
    VL_RTTI_IMPL(ReorderRVEdge, Reorder_ಅಂಚು)
ಸಾರ್ವಜನಿಕ:
    ReorderRVEdge(V3_ನಕ್ಷೆ* graphp, V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp, V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ)
        : Reorder_ಅಂಚು{graphp, fromp, ತುದಿ, CUTABLE} {}
    ~ReorderRVEdge() override = ಪೂರ್ವನಿಯೋಜಿತ;
    ತಾರ್ಕಿಕ followScoreboard() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ನಿಜ; }
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "green"; }
};

ವರ್ಗ Reorder_Scorebd_ಅಂಚು final : ಸಾರ್ವಜನಿಕ Reorder_ಅಂಚು {
    VL_RTTI_IMPL(Reorder_Scorebd_ಅಂಚು, Reorder_ಅಂಚು)
ಸಾರ್ವಜನಿಕ:
    Reorder_Scorebd_ಅಂಚು(V3_ನಕ್ಷೆ* graphp, V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp, V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ)
        : Reorder_ಅಂಚು{graphp, fromp, ತುದಿ, CUTABLE} {}
    ~Reorder_Scorebd_ಅಂಚು() override = ಪೂರ್ವನಿಯೋಜಿತ;
    ತಾರ್ಕಿಕ followScoreboard() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ನಿಜ; }
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "blue"; }
};

ವರ್ಗ Reorder_Strict_ಅಂಚು final : ಸಾರ್ವಜನಿಕ Reorder_ಅಂಚು {
    VL_RTTI_IMPL(Reorder_Strict_ಅಂಚು, Reorder_ಅಂಚು)
    // A strict order, based on the original statement order in the graph
    // The only non-cutable edge type
ಸಾರ್ವಜನಿಕ:
    Reorder_Strict_ಅಂಚು(V3_ನಕ್ಷೆ* graphp, V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp, V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ)
        : Reorder_ಅಂಚು{graphp, fromp, ತುದಿ, NOT_CUTABLE} {}
    ~Reorder_Strict_ಅಂಚು() override = ಪೂರ್ವನಿಯೋಜಿತ;
    ತಾರ್ಕಿಕ followScoreboard() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ನಿಜ; }
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "blue"; }
};

//######################################################################
// Reorder class functions

ವರ್ಗ ReorderVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> ReorderVarStdVertex*.  Usage var, 0=not set yet
    // AstVarScope::user2p  -> ReorderVarPostVertex*.  Delayed assignment var, 0=not set yet
    // Ast*::user3p         -> ReorderLogicVertex*.  Statement (temporary only)
    // Ast*::user4          -> int.  Current ordering number (reorderBlock usage)

    // STATE
    V3_ನಕ್ಷೆ* m_graphp = ಶೂನ್ಯಸೂಚಕ;  // Scoreboard of var usages/dependencies
    Reorder_Impure_ಶೃಂಗ* m_impureVtxp = ಶೂನ್ಯಸೂಚಕ;  // Element specifying PLI ordering
    ತಾರ್ಕಿಕ m_ಒಳ_Dly = ಸುಳ್ಳು;  // Inside ASSIGNDLY
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* m_noReorderWhy = ಶೂನ್ಯಸೂಚಕ;  // Reason we can't reorder
    std::ಸದಿಶ<Reorder_Logic_ಶೃಂಗ*> m_stmtStackps;  // Current statements being tracked

    // METHODS
    ಶೂನ್ಯ cleanup_ಖಂಡ_ನಕ್ಷೆ(Ast_ಗಂಟು* nodep) {
        // Transform the graph into what we need
        UINFO(5, "ReorderBlock " << nodep);

        // Simplify graph by removing redundant edges
        m_graphp->remove_Redundant_ಅಂಚುಗಳು_Max(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_nodup", ಸುಳ್ಳು);

        // Mark all the logic for this step by setting Vertex::user() to true
        m_graphp->userClearVertices();
        ಗಾಗಿ (Ast_ಗಂಟು* nextp = nodep; nextp; nextp = nextp->nextp()) {
            nextp->user3u().ಗಮ್ಯ<Reorder_Logic_ಶೃಂಗ*>()->user(ನಿಜ);
        }

        // New step
        Reorder_ಅಂಚು::increment_ಹೆಜ್ಜೆ();

        // If a var vertex has only inputs, it's a input-only node,
        // and can be ignored for coloring **this block only**
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_graphp->vertices()) {
            ಆದರೆ (!vtx.ಹೊರಹೋಗು_ಖಾಲಿಗಳು()) ಮುಂದುವರಿ;
            ಆದರೆ (!vtx.is<Reorder_Var_Std_ಶೃಂಗ>()) ಮುಂದುವರಿ;
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtx.ಒಳ_ಅಂಚುಗಳು()) ಅಂಚು.as<Reorder_ಅಂಚು>()->ಗಣ_Ignore_This_ಹೆಜ್ಜೆ();
        }

        // For reordering this single block only, mark all logic
        // vertexes not involved with this step as unimportant
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : m_graphp->vertices()) {
            ಆದರೆ (ಶೃಂಗ.user()) ಮುಂದುವರಿ;
            ಆದರೆ (!ಶೃಂಗ.is<Reorder_Logic_ಶೃಂಗ>()) ಮುಂದುವರಿ;
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : ಶೃಂಗ.ಒಳ_ಅಂಚುಗಳು()) ಅಂಚು.as<Reorder_ಅಂಚು>()->ಗಣ_Ignore_This_ಹೆಜ್ಜೆ();
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : ಶೃಂಗ.ಹೊರಹೋಗು_ಅಂಚುಗಳು())
                ಅಂಚು.as<Reorder_ಅಂಚು>()->ಗಣ_Ignore_This_ಹೆಜ್ಜೆ();
        }

        // Weak coloring to determine what needs to remain in order
        // This follows all step-relevant edges excluding PostEdges, which are done later
        m_graphp->weaklyConnected(&Reorder_ಅಂಚು::followScoreboard);

        // Add hard orderings between all nodes of same color, in the order they appeared
        std::unordered_map<uint32_t, Reorder_Logic_ಶೃಂಗ*> ಕೊನೆಯ_ದ_Color;
        ಗಾಗಿ (Ast_ಗಂಟು* currp = nodep; currp; currp = currp->nextp()) {
            Reorder_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = currp->user3u().ಗಮ್ಯ<Reorder_Logic_ಶೃಂಗ*>();
            ಸ್ಥಿರಾಂಕ uint32_t color = vtxp->color();
            ಆದರೆ (ಕೊನೆಯ_ದ_Color[color]) ಹೊಸ Reorder_Strict_ಅಂಚು{m_graphp, ಕೊನೆಯ_ದ_Color[color], vtxp};
            ಕೊನೆಯ_ದ_Color[color] = vtxp;
        }

        // And a real ordering to get the statements into something reasonable
        // We don't care if there's cutable violations here...
        // Non-cutable violations should be impossible; as those edges are program-order
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_pre", ಸುಳ್ಳು);
        m_graphp->acyclic(&Reorder_ಅಂಚು::followCyclic);
        m_graphp->rank(&Reorder_ಅಂಚು::followCyclic);  // Or order(), but that's more expensive
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_opt", ಸುಳ್ಳು);
    }

    ಶೂನ್ಯ reorder_ಖಂಡ(Ast_ಗಂಟು* nodep) {
        // Reorder statements in the completed graph

        // Map the rank numbers into nodes they associate with
        std::multimap<uint32_t, Ast_ಗಂಟು*> rankMap;
        ಪೂರ್ಣಾಂಕ curr_ಕ್ರಮ = 0;  // Existing sequence number of assignment
        ಗಾಗಿ (Ast_ಗಂಟು* currp = nodep; currp; currp = currp->nextp()) {
            ಸ್ಥಿರಾಂಕ Reorder_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = currp->user3u().ಗಮ್ಯ<Reorder_Logic_ಶೃಂಗ*>();
            rankMap.emplace(vtxp->rank(), currp);
            currp->user4(++curr_ಕ್ರಮ);  // Record current ordering
        }

        // Is the current ordering OK?
        ತಾರ್ಕಿಕ leaveAlone = ನಿಜ;
        uint64_t new_ಕ್ರಮ = 0;  // New sequence number of assignment
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಅಂಶ_2 : rankMap) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = ಅಂಶ_2.ಎರಡನೇ;
            ಆದರೆ (++new_ಕ್ರಮ != nextp->user4()) leaveAlone = ಸುಳ್ಳು;
        }
        ಆದರೆ (leaveAlone) {
            UINFO(6, "   No changes");
            ಹಿಂತಿರುಗಿ;
        }

        VNRelinker replaceHandle;  // Where to add the list
        Ast_ಗಂಟು* newListp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಅಂಶ_2 : rankMap) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = ಅಂಶ_2.ಎರಡನೇ;
            UINFO(6, "   New order: " << nextp);
            nextp->unlink_Fr_ಹಿಂಭಾಗ(nextp == nodep ? &replaceHandle : ಶೂನ್ಯಸೂಚಕ);
            newListp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(newListp, nextp);
        }
        replaceHandle.relink(newListp);
    }

    ಶೂನ್ಯ process_ಖಂಡ(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (m_noReorderWhy) ಹಿಂತಿರುಗಿ;

        // Empty lists are ignorable
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ;
        UASSERT_OBJ(nodep->ಮೊದಲ_Abovep(), nodep, "Node passed is in not head of list");
        UASSERT_OBJ(!nodep->user3p(), nodep, "Should not have a logic vertex");

        // It nothing to reorder with, just iterate
        ಆದರೆ (!nodep->nextp()) {
            iterate(nodep);
            ಹಿಂತಿರುಗಿ;
        }

        // Process it
        UINFO(9, "  processBlock " << nodep);

        // Iterate across current block, making the scoreboard
        ಗಾಗಿ (Ast_ಗಂಟು* currp = nodep; currp; currp = currp->nextp()) {
            // Create the logic vertex for this statement
            UASSERT_OBJ(!currp->user3p(), currp, "user3p should not be set");
            Reorder_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = ಹೊಸ Reorder_Logic_ಶೃಂಗ{m_graphp, currp};
            currp->user3p(vtxp);

            // Visit the statement - this can recursively reorder sub statements
            m_stmtStackps.ತಳ್ಳು_ಹಿಂಭಾಗ(vtxp);
            iterate(currp);
            m_stmtStackps.ತೆಗೆ_ಹಿಂಭಾಗ();
        }

        ಆದರೆ (m_noReorderWhy) {  // Jump or something nasty
            UINFO(9, "  NoReorderBlock because " << m_noReorderWhy);
            ಹಿಂತಿರುಗಿ;
        }

        // Reorder statements in this block
        cleanup_ಖಂಡ_ನಕ್ಷೆ(nodep);
        reorder_ಖಂಡ(nodep);

        // 'nodep' might no longer be the head of the list, rewind
        ಆಗ (nodep->backp()->nextp() == nodep) nodep = nodep->backp();

        // Delete vertexes and edges only applying to this block
        ಗಾಗಿ (Ast_ಗಂಟು* currp = nodep; currp; currp = currp->nextp()) {
            currp->user3u().ಗಮ್ಯ<Reorder_Logic_ಶೃಂಗ*>()->unlinkDelete(m_graphp);
            currp->user3p(ಶೂನ್ಯಸೂಚಕ);
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstAlways* nodep) override {
        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_ಒಳ_Dly);
        VL_RESTORER(m_noReorderWhy);

        V3_ನಕ್ಷೆ ನಕ್ಷೆ;
        m_graphp = &ನಕ್ಷೆ;
        m_impureVtxp = ಶೂನ್ಯಸೂಚಕ;
        m_ಒಳ_Dly = ಸುಳ್ಳು;
        m_noReorderWhy = ಶೂನ್ಯಸೂಚಕ;

        ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use user1_ಒಳ_Use;
        ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use user2_ಒಳ_Use;
        ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use user3_ಒಳ_Use;
        ಸ್ಥಿರಾಂಕ VNUser4_ಒಳ_Use user4_ಒಳ_Use;

        UINFOTREE(9, nodep, "", "alwIn:");
        process_ಖಂಡ(nodep->stmtsp());
        UINFOTREE(9, nodep, "", "alwOut");

        m_stmtStackps.clear();
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_If* nodep) override {
        ಆದರೆ (!m_graphp || m_noReorderWhy) ಹಿಂತಿರುಗಿ;
        iterate_And_ಮುಂದಿನ_Null(nodep->condp());
        process_ಖಂಡ(nodep->thensp());
        process_ಖಂಡ(nodep->elsesp());
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstJumpGo* nodep) override {
        ಆದರೆ (!m_graphp || m_noReorderWhy) ಹಿಂತಿರುಗಿ;
        m_noReorderWhy = "JumpGo";
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_CCall* nodep) override {
        ಆದರೆ (!m_graphp || m_noReorderWhy) ಹಿಂತಿರುಗಿ;
        // The callee is not inlined, so the variables it reads and writes carry no VarRef at
        // the call site, and the scoreboard would let assignments to them move across the
        // call. Treat any such call as a barrier.
        m_noReorderWhy = "CCall";
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override {
        ಆದರೆ (!m_graphp || m_noReorderWhy) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_ಒಳ_Dly);
        m_ಒಳ_Dly = ಸುಳ್ಳು;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Dly* nodep) override {
        ಆದರೆ (!m_graphp || m_noReorderWhy) ಹಿಂತಿರುಗಿ;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_ಒಳ_Dly);
        m_ಒಳ_Dly = ನಿಜ;
        iterate(nodep->lhsp());
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        ಆದರೆ (m_graphp) m_noReorderWhy = "Handle";
        ಭೇಟಿ(static_cast<Ast_ಗಂಟು*>(nodep));
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        ಆದರೆ (!m_graphp || m_noReorderWhy) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (m_stmtStackps.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;

        // Reads of constants can be ignored - TODO: This should be "constexpr", not run-time const
        ಆದರೆ (nodep->varp()->isConst()) ಹಿಂತಿರುಗಿ;

        // SPEEDUP: We add duplicate edges, that should be fixed

        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = nodep->varScopep();

        // Create vertexes for variable
        ಆದರೆ (!vscp->user1p()) vscp->user1p(ಹೊಸ Reorder_Var_Std_ಶೃಂಗ{m_graphp, vscp});
        Reorder_Var_Std_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vstdp = vscp->user1u().ಗಮ್ಯ<Reorder_Var_Std_ಶೃಂಗ*>();

        // Variable is read
        ಆದರೆ (nodep->access().isReadOnly()) {
            ಗಾಗಿ (Reorder_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_stmtStackps) {
                ಹೊಸ ReorderRVEdge{m_graphp, vtxp, vstdp};
            }
            ಹಿಂತಿರುಗಿ;
        }

        // Variable is written, not NBA
        ಆದರೆ (!m_ಒಳ_Dly) {
            ಗಾಗಿ (Reorder_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_stmtStackps) {
                ಹೊಸ ReorderLVEdge{m_graphp, vstdp, vtxp};
            }
            ಹಿಂತಿರುಗಿ;
        }

        // Variable is written by NBA
        ಆದರೆ (!vscp->user2p()) {
            Reorder_Var_Post_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vpostp = ಹೊಸ Reorder_Var_Post_ಶೃಂಗ{m_graphp, vscp};
            vscp->user2p(vpostp);
            ಹೊಸ Reorder_Post_ಅಂಚು{m_graphp, vstdp, vpostp};
        }
        Reorder_Var_Post_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vpostp = vscp->user2u().ಗಮ್ಯ<Reorder_Var_Post_ಶೃಂಗ*>();
        ಗಾಗಿ (Reorder_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_stmtStackps) {
            ಹೊಸ ReorderLVEdge{m_graphp, vpostp, vtxp};
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        // Outside AstAlways, just descend
        ಆದರೆ (!m_graphp) {
            iterateChildren(nodep);
            ಹಿಂತಿರುಗಿ;
        }

        // Early exit if decided not to reorder
        ಆದರೆ (m_noReorderWhy) ಹಿಂತಿರುಗಿ;

        // Timing control prevents reordering
        ಆದರೆ (nodep->isTimingControl()) {
            m_noReorderWhy = "TimingControl";
            ಹಿಂತಿರುಗಿ;
        }

        // Order all impure statements with other impure statements
        ಆದರೆ (!nodep->is_ಶುದ್ಧ()) {
            ಆದರೆ (!m_impureVtxp) m_impureVtxp = ಹೊಸ Reorder_Impure_ಶೃಂಗ{m_graphp, nodep};
            // This edge is only used to find weakly connected components, so one edge is enough
            ಗಾಗಿ (Reorder_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_stmtStackps) {
                ಹೊಸ Reorder_Scorebd_ಅಂಚು{m_graphp, m_impureVtxp, vtxp};
            }
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ ReorderVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReorderVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

}  // namespace

//######################################################################
// V3Reorder class functions

ಶೂನ್ಯ V3Reorder::reorderAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReorderVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("reorder", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
