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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Reorder.h"

#ضمن "V3EmitV.h"
#ضمن "V3Graph.h"
#ضمن "V3Stats.h"

#ضمن <string>
#ضمن <unordered_map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

نطاق {

//######################################################################
// Support classes

صنف Reorder_غوټه_راس VL_NOT_FINAL : عام V3_ګراف_راس {
    VL_RTTI_IMPL(Reorder_غوټه_راس, V3_ګراف_راس)
    Ast_غوټه* ثابت m_nodep;

محمي:
    Reorder_غوټه_راس(V3_ګراف* graphp, Ast_غوټه* nodep)
        : V3_ګراف_راس{graphp}
        , m_nodep{nodep} {}
    ~Reorder_غوټه_راس() override = افتراضي;
    // ACCESSORS
    // Do not make accessor for nodep(),  It may change due to
    // reordering a lower block, but we don't repair it
    std::string name() ثابت override {
        std::string str = cvt_منزل_Hex(m_nodep) + '\n';
        إذا (AstVarScope* ثابت vscp = VN_CAST(m_nodep, VarScope)) {
            str += vscp->prettyName();
        } وإلا {
            str += V3EmitV::debug_Verilog_For_ونه(m_nodep);
            str = VString::quoteBackslash(str);
            str = VString::quoteAny(str, '"', '\\');
            str = VString::replaceSubstr(str, "\n", "\\l");
        }
        أرجع str;
    }
    File_کرښه* fileline() ثابت override { أرجع nodep()->fileline(); }
    std::string dotShape() ثابت override { أرجع VN_IS(m_nodep, VarScope) ? "ellipse" : "box"; }

عام:
    مجازي Ast_غوټه* nodep() ثابت { أرجع m_nodep; }
};

صنف Reorder_Impure_راس final : عام Reorder_غوټه_راس {
    VL_RTTI_IMPL(Reorder_Impure_راس, Reorder_غوټه_راس)
عام:
    څرګند Reorder_Impure_راس(V3_ګراف* graphp, Ast_غوټه* nodep)
        : Reorder_غوټه_راس{graphp, nodep} {}
    ~Reorder_Impure_راس() override = افتراضي;
    string name() ثابت override VL_MT_STABLE { أرجع "*IMPURE*"; }
    string dotColor() ثابت override { أرجع "red"; }
};

صنف Reorder_Logic_راس final : عام Reorder_غوټه_راس {
    VL_RTTI_IMPL(Reorder_Logic_راس, Reorder_غوټه_راس)
عام:
    Reorder_Logic_راس(V3_ګراف* graphp, Ast_غوټه* nodep)
        : Reorder_غوټه_راس{graphp, nodep} {}
    ~Reorder_Logic_راس() override = افتراضي;
    string dotColor() ثابت override { أرجع "black"; }
};

صنف Reorder_Var_Std_راس final : عام Reorder_غوټه_راس {
    VL_RTTI_IMPL(Reorder_Var_Std_راس, Reorder_غوټه_راس)
عام:
    Reorder_Var_Std_راس(V3_ګراف* graphp, AstVarScope* nodep)
        : Reorder_غوټه_راس{graphp, nodep} {}
    ~Reorder_Var_Std_راس() override = افتراضي;
    string dotColor() ثابت override { أرجع "blue"; }
};

صنف Reorder_Var_Post_راس final : عام Reorder_غوټه_راس {
    VL_RTTI_IMPL(Reorder_Var_Post_راس, Reorder_غوټه_راس)
عام:
    Reorder_Var_Post_راس(V3_ګراف* graphp, AstVarScope* nodep)
        : Reorder_غوټه_راس{graphp, nodep} {}
    ~Reorder_Var_Post_راس() override = افتراضي;
    string name() ثابت override { أرجع "POST "s + Reorder_غوټه_راس::name(); }
    string dotColor() ثابت override { أرجع "green"; }
};

//######################################################################
// Edge types

صنف Reorder_څنډه VL_NOT_FINAL : عام V3_ګراف_څنډه {
    VL_RTTI_IMPL(Reorder_څنډه, V3_ګراف_څنډه)
    uint32_t m_ignore_دننه_ګام = 0;  // Step number that if set to, causes this edge to be ignored
    ساكن uint32_t s_ګام_عدد;  // Global step number
محمي:
    ساكن constexpr صحيح وزن_نورمال = 10;
    Reorder_څنډه(V3_ګراف* graphp, V3_ګراف_راس* fromp, V3_ګراف_راس* سر, منطقي cutable)
        : V3_ګراف_څنډه{graphp, fromp, سر, وزن_نورمال, cutable} {}
    ~Reorder_څنډه() override = افتراضي;

    مجازي منطقي followScoreboard() ثابت = 0;

    std::string dotStyle() ثابت override {
        أرجع ignore_This_ګام() ? "dotted" : V3_ګراف_څنډه::dotStyle();
    }

عام:
    // Iterator for graph functions
    ساكن فراغ increment_ګام() { ++s_ګام_عدد; }
    منطقي ignore_This_ګام() ثابت { أرجع m_ignore_دننه_ګام == s_ګام_عدد; }
    فراغ سېټ_Ignore_This_ګام() { m_ignore_دننه_ګام = s_ګام_عدد; }

    ساكن منطقي followScoreboard(ثابت V3_ګراف_څنډه* edgep) {
        ثابت Reorder_څنډه& څنډه = *edgep->as<Reorder_څنډه>();
        أرجع !څنډه.ignore_This_ګام() && څنډه.followScoreboard();
    }
    ساكن منطقي followCyclic(ثابت V3_ګراف_څنډه* edgep) {
        ثابت Reorder_څنډه& څنډه = *edgep->as<Reorder_څنډه>();
        أرجع !څنډه.ignore_This_ګام();
    }
};
uint32_t Reorder_څنډه::s_ګام_عدد = 0;

صنف Reorder_Post_څنډه final : عام Reorder_څنډه {
    VL_RTTI_IMPL(Reorder_Post_څنډه, Reorder_څنډه)
عام:
    Reorder_Post_څنډه(V3_ګراف* graphp, V3_ګراف_راس* fromp, V3_ګراف_راس* سر)
        : Reorder_څنډه{graphp, fromp, سر, CUTABLE} {}
    ~Reorder_Post_څنډه() override = افتراضي;
    منطقي followScoreboard() ثابت override { أرجع خطأ; }
    string dotColor() ثابت override { أرجع "khaki"; }
};

صنف ReorderLVEdge final : عام Reorder_څنډه {
    VL_RTTI_IMPL(ReorderLVEdge, Reorder_څنډه)
عام:
    ReorderLVEdge(V3_ګراف* graphp, V3_ګراف_راس* fromp, V3_ګراف_راس* سر)
        : Reorder_څنډه{graphp, fromp, سر, CUTABLE} {}
    ~ReorderLVEdge() override = افتراضي;
    منطقي followScoreboard() ثابت override { أرجع ریښتیا; }
    string dotColor() ثابت override { أرجع "yellowGreen"; }
};

صنف ReorderRVEdge final : عام Reorder_څنډه {
    VL_RTTI_IMPL(ReorderRVEdge, Reorder_څنډه)
عام:
    ReorderRVEdge(V3_ګراف* graphp, V3_ګراف_راس* fromp, V3_ګراف_راس* سر)
        : Reorder_څنډه{graphp, fromp, سر, CUTABLE} {}
    ~ReorderRVEdge() override = افتراضي;
    منطقي followScoreboard() ثابت override { أرجع ریښتیا; }
    string dotColor() ثابت override { أرجع "green"; }
};

صنف Reorder_Scorebd_څنډه final : عام Reorder_څنډه {
    VL_RTTI_IMPL(Reorder_Scorebd_څنډه, Reorder_څنډه)
عام:
    Reorder_Scorebd_څنډه(V3_ګراف* graphp, V3_ګراف_راس* fromp, V3_ګراف_راس* سر)
        : Reorder_څنډه{graphp, fromp, سر, CUTABLE} {}
    ~Reorder_Scorebd_څنډه() override = افتراضي;
    منطقي followScoreboard() ثابت override { أرجع ریښتیا; }
    string dotColor() ثابت override { أرجع "blue"; }
};

صنف Reorder_Strict_څنډه final : عام Reorder_څنډه {
    VL_RTTI_IMPL(Reorder_Strict_څنډه, Reorder_څنډه)
    // A strict order, based on the original statement order in the graph
    // The only non-cutable edge type
عام:
    Reorder_Strict_څنډه(V3_ګراف* graphp, V3_ګراف_راس* fromp, V3_ګراف_راس* سر)
        : Reorder_څنډه{graphp, fromp, سر, NOT_CUTABLE} {}
    ~Reorder_Strict_څنډه() override = افتراضي;
    منطقي followScoreboard() ثابت override { أرجع ریښتیا; }
    string dotColor() ثابت override { أرجع "blue"; }
};

//######################################################################
// Reorder class functions

صنف ReorderVisitor final : عام VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> ReorderVarStdVertex*.  Usage var, 0=not set yet
    // AstVarScope::user2p  -> ReorderVarPostVertex*.  Delayed assignment var, 0=not set yet
    // Ast*::user3p         -> ReorderLogicVertex*.  Statement (temporary only)
    // Ast*::user4          -> int.  Current ordering number (reorderBlock usage)

    // STATE
    V3_ګراف* m_graphp = تش_اشاره;  // Scoreboard of var usages/dependencies
    Reorder_Impure_راس* m_impureVtxp = تش_اشاره;  // Element specifying PLI ordering
    منطقي m_دننه_Dly = خطأ;  // Inside ASSIGNDLY
    ثابت حرف* m_noReorderWhy = تش_اشاره;  // Reason we can't reorder
    std::ویکټور<Reorder_Logic_راس*> m_stmtStackps;  // Current statements being tracked

    // METHODS
    فراغ cleanup_بلاک_ګراف(Ast_غوټه* nodep) {
        // Transform the graph into what we need
        UINFO(5, "ReorderBlock " << nodep);

        // Simplify graph by removing redundant edges
        m_graphp->remove_Redundant_څنډې_Max(&V3_ګراف_څنډه::followAlwaysTrue);
        إذا (dump_ګراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_nodup", خطأ);

        // Mark all the logic for this step by setting Vertex::user() to true
        m_graphp->userClearVertices();
        لكل (Ast_غوټه* nextp = nodep; nextp; nextp = nextp->nextp()) {
            nextp->user3u().منزل<Reorder_Logic_راس*>()->user(ریښتیا);
        }

        // New step
        Reorder_څنډه::increment_ګام();

        // If a var vertex has only inputs, it's a input-only node,
        // and can be ignored for coloring **this block only**
        لكل (V3_ګراف_راس& vtx : m_graphp->vertices()) {
            إذا (!vtx.بهر_تشه()) استمر;
            إذا (!vtx.is<Reorder_Var_Std_راس>()) استمر;
            لكل (V3_ګراف_څنډه& څنډه : vtx.دننه_څنډې()) څنډه.as<Reorder_څنډه>()->سېټ_Ignore_This_ګام();
        }

        // For reordering this single block only, mark all logic
        // vertexes not involved with this step as unimportant
        لكل (V3_ګراف_راس& راس : m_graphp->vertices()) {
            إذا (راس.user()) استمر;
            إذا (!راس.is<Reorder_Logic_راس>()) استمر;
            لكل (V3_ګراف_څنډه& څنډه : راس.دننه_څنډې()) څنډه.as<Reorder_څنډه>()->سېټ_Ignore_This_ګام();
            لكل (V3_ګراف_څنډه& څنډه : راس.بهر_څنډې())
                څنډه.as<Reorder_څنډه>()->سېټ_Ignore_This_ګام();
        }

        // Weak coloring to determine what needs to remain in order
        // This follows all step-relevant edges excluding PostEdges, which are done later
        m_graphp->weaklyConnected(&Reorder_څنډه::followScoreboard);

        // Add hard orderings between all nodes of same color, in the order they appeared
        std::unordered_map<uint32_t, Reorder_Logic_راس*> وروستی_د_Color;
        لكل (Ast_غوټه* currp = nodep; currp; currp = currp->nextp()) {
            Reorder_Logic_راس* ثابت vtxp = currp->user3u().منزل<Reorder_Logic_راس*>();
            ثابت uint32_t color = vtxp->color();
            إذا (وروستی_د_Color[color]) جديد Reorder_Strict_څنډه{m_graphp, وروستی_د_Color[color], vtxp};
            وروستی_د_Color[color] = vtxp;
        }

        // And a real ordering to get the statements into something reasonable
        // We don't care if there's cutable violations here...
        // Non-cutable violations should be impossible; as those edges are program-order
        إذا (dump_ګراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_pre", خطأ);
        m_graphp->acyclic(&Reorder_څنډه::followCyclic);
        m_graphp->rank(&Reorder_څنډه::followCyclic);  // Or order(), but that's more expensive
        إذا (dump_ګراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_opt", خطأ);
    }

    فراغ reorder_بلاک(Ast_غوټه* nodep) {
        // Reorder statements in the completed graph

        // Map the rank numbers into nodes they associate with
        std::multimap<uint32_t, Ast_غوټه*> rankMap;
        صحيح curr_ترتیب = 0;  // Existing sequence number of assignment
        لكل (Ast_غوټه* currp = nodep; currp; currp = currp->nextp()) {
            ثابت Reorder_Logic_راس* ثابت vtxp = currp->user3u().منزل<Reorder_Logic_راس*>();
            rankMap.emplace(vtxp->rank(), currp);
            currp->user4(++curr_ترتیب);  // Record current ordering
        }

        // Is the current ordering OK?
        منطقي leaveAlone = ریښتیا;
        uint64_t new_ترتیب = 0;  // New sequence number of assignment
        لكل (ثابت خپلکاری& توکی_2 : rankMap) {
            ثابت Ast_غوټه* ثابت nextp = توکی_2.دوهم;
            إذا (++new_ترتیب != nextp->user4()) leaveAlone = خطأ;
        }
        إذا (leaveAlone) {
            UINFO(6, "   No changes");
            أرجع;
        }

        VNRelinker replaceHandle;  // Where to add the list
        Ast_غوټه* newListp = تش_اشاره;
        لكل (ثابت خپلکاری& توکی_2 : rankMap) {
            Ast_غوټه* ثابت nextp = توکی_2.دوهم;
            UINFO(6, "   New order: " << nextp);
            nextp->unlink_Fr_شا(nextp == nodep ? &replaceHandle : تش_اشاره);
            newListp = Ast_غوټه::جمع_کړه_بل(newListp, nextp);
        }
        replaceHandle.relink(newListp);
    }

    فراغ process_بلاک(Ast_غوټه* nodep) {
        إذا (m_noReorderWhy) أرجع;

        // Empty lists are ignorable
        إذا (!nodep) أرجع;
        UASSERT_OBJ(nodep->لومړی_Abovep(), nodep, "Node passed is in not head of list");
        UASSERT_OBJ(!nodep->user3p(), nodep, "Should not have a logic vertex");

        // It nothing to reorder with, just iterate
        إذا (!nodep->nextp()) {
            iterate(nodep);
            أرجع;
        }

        // Process it
        UINFO(9, "  processBlock " << nodep);

        // Iterate across current block, making the scoreboard
        لكل (Ast_غوټه* currp = nodep; currp; currp = currp->nextp()) {
            // Create the logic vertex for this statement
            UASSERT_OBJ(!currp->user3p(), currp, "user3p should not be set");
            Reorder_Logic_راس* ثابت vtxp = جديد Reorder_Logic_راس{m_graphp, currp};
            currp->user3p(vtxp);

            // Visit the statement - this can recursively reorder sub statements
            m_stmtStackps.ټېل_شا(vtxp);
            iterate(currp);
            m_stmtStackps.راوباسه_شا();
        }

        إذا (m_noReorderWhy) {  // Jump or something nasty
            UINFO(9, "  NoReorderBlock because " << m_noReorderWhy);
            أرجع;
        }

        // Reorder statements in this block
        cleanup_بلاک_ګراف(nodep);
        reorder_بلاک(nodep);

        // 'nodep' might no longer be the head of the list, rewind
        طالما (nodep->backp()->nextp() == nodep) nodep = nodep->backp();

        // Delete vertexes and edges only applying to this block
        لكل (Ast_غوټه* currp = nodep; currp; currp = currp->nextp()) {
            currp->user3u().منزل<Reorder_Logic_راس*>()->unlinkDelete(m_graphp);
            currp->user3p(تش_اشاره);
        }
    }

    // VISITORS
    فراغ لیدنه(AstAlways* nodep) override {
        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_دننه_Dly);
        VL_RESTORER(m_noReorderWhy);

        V3_ګراف ګراف;
        m_graphp = &ګراف;
        m_impureVtxp = تش_اشاره;
        m_دننه_Dly = خطأ;
        m_noReorderWhy = تش_اشاره;

        ثابت VNUser1_دننه_Use user1_دننه_Use;
        ثابت VNUser2_دننه_Use user2_دننه_Use;
        ثابت VNUser3_دننه_Use user3_دننه_Use;
        ثابت VNUser4_دننه_Use user4_دننه_Use;

        UINFOTREE(9, nodep, "", "alwIn:");
        process_بلاک(nodep->stmtsp());
        UINFOTREE(9, nodep, "", "alwOut");

        m_stmtStackps.clear();
    }

    فراغ لیدنه(Ast_غوټه_If* nodep) override {
        إذا (!m_graphp || m_noReorderWhy) أرجع;
        iterate_And_بل_Null(nodep->condp());
        process_بلاک(nodep->thensp());
        process_بلاک(nodep->elsesp());
    }

    فراغ لیدنه(AstJumpGo* nodep) override {
        إذا (!m_graphp || m_noReorderWhy) أرجع;
        m_noReorderWhy = "JumpGo";
        iterateChildren(nodep);
    }

    فراغ لیدنه(Ast_غوټه_CCall* nodep) override {
        إذا (!m_graphp || m_noReorderWhy) أرجع;
        // The callee is not inlined, so the variables it reads and writes carry no VarRef at
        // the call site, and the scoreboard would let assignments to them move across the
        // call. Treat any such call as a barrier.
        m_noReorderWhy = "CCall";
        iterateChildren(nodep);
    }

    فراغ لیدنه(Ast_بیان_Stmt* nodep) override {
        إذا (!m_graphp || m_noReorderWhy) أرجع;
        VL_RESTORER(m_دننه_Dly);
        m_دننه_Dly = خطأ;
        iterateChildren(nodep);
    }

    فراغ لیدنه(Ast_ټاکل_Dly* nodep) override {
        إذا (!m_graphp || m_noReorderWhy) أرجع;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        إذا (Ast_غوټه* ثابت controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_دننه_Dly);
        m_دننه_Dly = ریښتیا;
        iterate(nodep->lhsp());
    }

    فراغ لیدنه(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        إذا (m_graphp) m_noReorderWhy = "Handle";
        لیدنه(static_cast<Ast_غوټه*>(nodep));
    }

    فراغ لیدنه(AstVarRef* nodep) override {
        إذا (!m_graphp || m_noReorderWhy) أرجع;
        إذا (m_stmtStackps.تشه()) أرجع;

        // Reads of constants can be ignored - TODO: This should be "constexpr", not run-time const
        إذا (nodep->varp()->isConst()) أرجع;

        // SPEEDUP: We add duplicate edges, that should be fixed

        AstVarScope* ثابت vscp = nodep->varScopep();

        // Create vertexes for variable
        إذا (!vscp->user1p()) vscp->user1p(جديد Reorder_Var_Std_راس{m_graphp, vscp});
        Reorder_Var_Std_راس* ثابت vstdp = vscp->user1u().منزل<Reorder_Var_Std_راس*>();

        // Variable is read
        إذا (nodep->access().isReadOnly()) {
            لكل (Reorder_Logic_راس* ثابت vtxp : m_stmtStackps) {
                جديد ReorderRVEdge{m_graphp, vtxp, vstdp};
            }
            أرجع;
        }

        // Variable is written, not NBA
        إذا (!m_دننه_Dly) {
            لكل (Reorder_Logic_راس* ثابت vtxp : m_stmtStackps) {
                جديد ReorderLVEdge{m_graphp, vstdp, vtxp};
            }
            أرجع;
        }

        // Variable is written by NBA
        إذا (!vscp->user2p()) {
            Reorder_Var_Post_راس* ثابت vpostp = جديد Reorder_Var_Post_راس{m_graphp, vscp};
            vscp->user2p(vpostp);
            جديد Reorder_Post_څنډه{m_graphp, vstdp, vpostp};
        }
        Reorder_Var_Post_راس* ثابت vpostp = vscp->user2u().منزل<Reorder_Var_Post_راس*>();
        لكل (Reorder_Logic_راس* ثابت vtxp : m_stmtStackps) {
            جديد ReorderLVEdge{m_graphp, vpostp, vtxp};
        }
    }

    فراغ لیدنه(Ast_غوټه* nodep) override {
        // Outside AstAlways, just descend
        إذا (!m_graphp) {
            iterateChildren(nodep);
            أرجع;
        }

        // Early exit if decided not to reorder
        إذا (m_noReorderWhy) أرجع;

        // Timing control prevents reordering
        إذا (nodep->isTimingControl()) {
            m_noReorderWhy = "TimingControl";
            أرجع;
        }

        // Order all impure statements with other impure statements
        إذا (!nodep->is_خالص()) {
            إذا (!m_impureVtxp) m_impureVtxp = جديد Reorder_Impure_راس{m_graphp, nodep};
            // This edge is only used to find weakly connected components, so one edge is enough
            لكل (Reorder_Logic_راس* ثابت vtxp : m_stmtStackps) {
                جديد Reorder_Scorebd_څنډه{m_graphp, m_impureVtxp, vtxp};
            }
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
عام:
    څرګند ReorderVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReorderVisitor() override = افتراضي;
};

}  // namespace

//######################################################################
// V3Reorder class functions

فراغ V3Reorder::reorderAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReorderVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("reorder", 0, dump_ونه_Either_Level() >= 3);
}
