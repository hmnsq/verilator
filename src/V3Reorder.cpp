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

صنف Reorder_گرہ_راس VL_NOT_FINAL : عام V3_گراف_راس {
    VL_RTTI_IMPL(Reorder_گرہ_راس, V3_گراف_راس)
    Ast_گرہ* ثابت m_nodep;

محمي:
    Reorder_گرہ_راس(V3_گراف* graphp, Ast_گرہ* nodep)
        : V3_گراف_راس{graphp}
        , m_nodep{nodep} {}
    ~Reorder_گرہ_راس() override = افتراضي;
    // ACCESSORS
    // Do not make accessor for nodep(),  It may change due to
    // reordering a lower block, but we don't repair it
    std::string name() ثابت override {
        std::string str = cvt_منزل_Hex(m_nodep) + '\n';
        إذا (AstVarScope* ثابت vscp = VN_CAST(m_nodep, VarScope)) {
            str += vscp->prettyName();
        } وإلا {
            str += V3EmitV::debug_Verilog_For_درخت(m_nodep);
            str = VString::quoteBackslash(str);
            str = VString::quoteAny(str, '"', '\\');
            str = VString::replaceSubstr(str, "\n", "\\l");
        }
        أرجع str;
    }
    File_سطر* fileline() ثابت override { أرجع nodep()->fileline(); }
    std::string dotShape() ثابت override { أرجع VN_IS(m_nodep, VarScope) ? "ellipse" : "box"; }

عام:
    مجازی Ast_گرہ* nodep() ثابت { أرجع m_nodep; }
};

صنف Reorder_Impure_راس final : عام Reorder_گرہ_راس {
    VL_RTTI_IMPL(Reorder_Impure_راس, Reorder_گرہ_راس)
عام:
    واضح Reorder_Impure_راس(V3_گراف* graphp, Ast_گرہ* nodep)
        : Reorder_گرہ_راس{graphp, nodep} {}
    ~Reorder_Impure_راس() override = افتراضي;
    string name() ثابت override VL_MT_STABLE { أرجع "*IMPURE*"; }
    string dotColor() ثابت override { أرجع "red"; }
};

صنف Reorder_Logic_راس final : عام Reorder_گرہ_راس {
    VL_RTTI_IMPL(Reorder_Logic_راس, Reorder_گرہ_راس)
عام:
    Reorder_Logic_راس(V3_گراف* graphp, Ast_گرہ* nodep)
        : Reorder_گرہ_راس{graphp, nodep} {}
    ~Reorder_Logic_راس() override = افتراضي;
    string dotColor() ثابت override { أرجع "black"; }
};

صنف Reorder_Var_Std_راس final : عام Reorder_گرہ_راس {
    VL_RTTI_IMPL(Reorder_Var_Std_راس, Reorder_گرہ_راس)
عام:
    Reorder_Var_Std_راس(V3_گراف* graphp, AstVarScope* nodep)
        : Reorder_گرہ_راس{graphp, nodep} {}
    ~Reorder_Var_Std_راس() override = افتراضي;
    string dotColor() ثابت override { أرجع "blue"; }
};

صنف Reorder_Var_Post_راس final : عام Reorder_گرہ_راس {
    VL_RTTI_IMPL(Reorder_Var_Post_راس, Reorder_گرہ_راس)
عام:
    Reorder_Var_Post_راس(V3_گراف* graphp, AstVarScope* nodep)
        : Reorder_گرہ_راس{graphp, nodep} {}
    ~Reorder_Var_Post_راس() override = افتراضي;
    string name() ثابت override { أرجع "POST "s + Reorder_گرہ_راس::name(); }
    string dotColor() ثابت override { أرجع "green"; }
};

//######################################################################
// Edge types

صنف Reorder_کنارا VL_NOT_FINAL : عام V3_گراف_کنارا {
    VL_RTTI_IMPL(Reorder_کنارا, V3_گراف_کنارا)
    uint32_t m_ignore_وچ_قدم = 0;  // Step number that if set to, causes this edge to be ignored
    ساكن uint32_t s_قدم_عدد;  // Global step number
محمي:
    ساكن constexpr صحيح بھار_عام = 10;
    Reorder_کنارا(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی, منطقي cutable)
        : V3_گراف_کنارا{graphp, fromp, چوٹی, بھار_عام, cutable} {}
    ~Reorder_کنارا() override = افتراضي;

    مجازی منطقي followScoreboard() ثابت = 0;

    std::string dotStyle() ثابت override {
        أرجع ignore_This_قدم() ? "dotted" : V3_گراف_کنارا::dotStyle();
    }

عام:
    // Iterator for graph functions
    ساكن فراغ increment_قدم() { ++s_قدم_عدد; }
    منطقي ignore_This_قدم() ثابت { أرجع m_ignore_وچ_قدم == s_قدم_عدد; }
    فراغ سیٹ_Ignore_This_قدم() { m_ignore_وچ_قدم = s_قدم_عدد; }

    ساكن منطقي followScoreboard(ثابت V3_گراف_کنارا* edgep) {
        ثابت Reorder_کنارا& کنارا = *edgep->as<Reorder_کنارا>();
        أرجع !کنارا.ignore_This_قدم() && کنارا.followScoreboard();
    }
    ساكن منطقي followCyclic(ثابت V3_گراف_کنارا* edgep) {
        ثابت Reorder_کنارا& کنارا = *edgep->as<Reorder_کنارا>();
        أرجع !کنارا.ignore_This_قدم();
    }
};
uint32_t Reorder_کنارا::s_قدم_عدد = 0;

صنف Reorder_Post_کنارا final : عام Reorder_کنارا {
    VL_RTTI_IMPL(Reorder_Post_کنارا, Reorder_کنارا)
عام:
    Reorder_Post_کنارا(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی)
        : Reorder_کنارا{graphp, fromp, چوٹی, CUTABLE} {}
    ~Reorder_Post_کنارا() override = افتراضي;
    منطقي followScoreboard() ثابت override { أرجع خطأ; }
    string dotColor() ثابت override { أرجع "khaki"; }
};

صنف ReorderLVEdge final : عام Reorder_کنارا {
    VL_RTTI_IMPL(ReorderLVEdge, Reorder_کنارا)
عام:
    ReorderLVEdge(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی)
        : Reorder_کنارا{graphp, fromp, چوٹی, CUTABLE} {}
    ~ReorderLVEdge() override = افتراضي;
    منطقي followScoreboard() ثابت override { أرجع سچ; }
    string dotColor() ثابت override { أرجع "yellowGreen"; }
};

صنف ReorderRVEdge final : عام Reorder_کنارا {
    VL_RTTI_IMPL(ReorderRVEdge, Reorder_کنارا)
عام:
    ReorderRVEdge(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی)
        : Reorder_کنارا{graphp, fromp, چوٹی, CUTABLE} {}
    ~ReorderRVEdge() override = افتراضي;
    منطقي followScoreboard() ثابت override { أرجع سچ; }
    string dotColor() ثابت override { أرجع "green"; }
};

صنف Reorder_Scorebd_کنارا final : عام Reorder_کنارا {
    VL_RTTI_IMPL(Reorder_Scorebd_کنارا, Reorder_کنارا)
عام:
    Reorder_Scorebd_کنارا(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی)
        : Reorder_کنارا{graphp, fromp, چوٹی, CUTABLE} {}
    ~Reorder_Scorebd_کنارا() override = افتراضي;
    منطقي followScoreboard() ثابت override { أرجع سچ; }
    string dotColor() ثابت override { أرجع "blue"; }
};

صنف Reorder_Strict_کنارا final : عام Reorder_کنارا {
    VL_RTTI_IMPL(Reorder_Strict_کنارا, Reorder_کنارا)
    // A strict order, based on the original statement order in the graph
    // The only non-cutable edge type
عام:
    Reorder_Strict_کنارا(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی)
        : Reorder_کنارا{graphp, fromp, چوٹی, NOT_CUTABLE} {}
    ~Reorder_Strict_کنارا() override = افتراضي;
    منطقي followScoreboard() ثابت override { أرجع سچ; }
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
    V3_گراف* m_graphp = خالی_سوچک;  // Scoreboard of var usages/dependencies
    Reorder_Impure_راس* m_impureVtxp = خالی_سوچک;  // Element specifying PLI ordering
    منطقي m_وچ_Dly = خطأ;  // Inside ASSIGNDLY
    ثابت حرف* m_noReorderWhy = خالی_سوچک;  // Reason we can't reorder
    std::سمتیہ<Reorder_Logic_راس*> m_stmtStackps;  // Current statements being tracked

    // METHODS
    فراغ cleanup_بلاک_گراف(Ast_گرہ* nodep) {
        // Transform the graph into what we need
        UINFO(5, "ReorderBlock " << nodep);

        // Simplify graph by removing redundant edges
        m_graphp->remove_Redundant_کنارے_Max(&V3_گراف_کنارا::followAlwaysTrue);
        إذا (dump_گراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_nodup", خطأ);

        // Mark all the logic for this step by setting Vertex::user() to true
        m_graphp->userClearVertices();
        لكل (Ast_گرہ* nextp = nodep; nextp; nextp = nextp->nextp()) {
            nextp->user3u().منزل<Reorder_Logic_راس*>()->user(سچ);
        }

        // New step
        Reorder_کنارا::increment_قدم();

        // If a var vertex has only inputs, it's a input-only node,
        // and can be ignored for coloring **this block only**
        لكل (V3_گراف_راس& vtx : m_graphp->vertices()) {
            إذا (!vtx.باہرلا_خالی_تھاواں()) استمر;
            إذا (!vtx.is<Reorder_Var_Std_راس>()) استمر;
            لكل (V3_گراف_کنارا& کنارا : vtx.وچ_کنارے()) کنارا.as<Reorder_کنارا>()->سیٹ_Ignore_This_قدم();
        }

        // For reordering this single block only, mark all logic
        // vertexes not involved with this step as unimportant
        لكل (V3_گراف_راس& راس : m_graphp->vertices()) {
            إذا (راس.user()) استمر;
            إذا (!راس.is<Reorder_Logic_راس>()) استمر;
            لكل (V3_گراف_کنارا& کنارا : راس.وچ_کنارے()) کنارا.as<Reorder_کنارا>()->سیٹ_Ignore_This_قدم();
            لكل (V3_گراف_کنارا& کنارا : راس.باہرلا_کنارے())
                کنارا.as<Reorder_کنارا>()->سیٹ_Ignore_This_قدم();
        }

        // Weak coloring to determine what needs to remain in order
        // This follows all step-relevant edges excluding PostEdges, which are done later
        m_graphp->weaklyConnected(&Reorder_کنارا::followScoreboard);

        // Add hard orderings between all nodes of same color, in the order they appeared
        std::unordered_map<uint32_t, Reorder_Logic_راس*> آخری_دا_Color;
        لكل (Ast_گرہ* currp = nodep; currp; currp = currp->nextp()) {
            Reorder_Logic_راس* ثابت vtxp = currp->user3u().منزل<Reorder_Logic_راس*>();
            ثابت uint32_t color = vtxp->color();
            إذا (آخری_دا_Color[color]) جديد Reorder_Strict_کنارا{m_graphp, آخری_دا_Color[color], vtxp};
            آخری_دا_Color[color] = vtxp;
        }

        // And a real ordering to get the statements into something reasonable
        // We don't care if there's cutable violations here...
        // Non-cutable violations should be impossible; as those edges are program-order
        إذا (dump_گراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_pre", خطأ);
        m_graphp->acyclic(&Reorder_کنارا::followCyclic);
        m_graphp->rank(&Reorder_کنارا::followCyclic);  // Or order(), but that's more expensive
        إذا (dump_گراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_opt", خطأ);
    }

    فراغ reorder_بلاک(Ast_گرہ* nodep) {
        // Reorder statements in the completed graph

        // Map the rank numbers into nodes they associate with
        std::multimap<uint32_t, Ast_گرہ*> rankMap;
        صحيح curr_ترتیب = 0;  // Existing sequence number of assignment
        لكل (Ast_گرہ* currp = nodep; currp; currp = currp->nextp()) {
            ثابت Reorder_Logic_راس* ثابت vtxp = currp->user3u().منزل<Reorder_Logic_راس*>();
            rankMap.emplace(vtxp->rank(), currp);
            currp->user4(++curr_ترتیب);  // Record current ordering
        }

        // Is the current ordering OK?
        منطقي leaveAlone = سچ;
        uint64_t new_ترتیب = 0;  // New sequence number of assignment
        لكل (ثابت خودکار& شے_2 : rankMap) {
            ثابت Ast_گرہ* ثابت nextp = شے_2.دوجا;
            إذا (++new_ترتیب != nextp->user4()) leaveAlone = خطأ;
        }
        إذا (leaveAlone) {
            UINFO(6, "   No changes");
            أرجع;
        }

        VNRelinker replaceHandle;  // Where to add the list
        Ast_گرہ* newListp = خالی_سوچک;
        لكل (ثابت خودکار& شے_2 : rankMap) {
            Ast_گرہ* ثابت nextp = شے_2.دوجا;
            UINFO(6, "   New order: " << nextp);
            nextp->unlink_Fr_پٹھ(nextp == nodep ? &replaceHandle : خالی_سوچک);
            newListp = Ast_گرہ::جوڑو_اگلا(newListp, nextp);
        }
        replaceHandle.relink(newListp);
    }

    فراغ process_بلاک(Ast_گرہ* nodep) {
        إذا (m_noReorderWhy) أرجع;

        // Empty lists are ignorable
        إذا (!nodep) أرجع;
        UASSERT_OBJ(nodep->پہلا_Abovep(), nodep, "Node passed is in not head of list");
        UASSERT_OBJ(!nodep->user3p(), nodep, "Should not have a logic vertex");

        // It nothing to reorder with, just iterate
        إذا (!nodep->nextp()) {
            iterate(nodep);
            أرجع;
        }

        // Process it
        UINFO(9, "  processBlock " << nodep);

        // Iterate across current block, making the scoreboard
        لكل (Ast_گرہ* currp = nodep; currp; currp = currp->nextp()) {
            // Create the logic vertex for this statement
            UASSERT_OBJ(!currp->user3p(), currp, "user3p should not be set");
            Reorder_Logic_راس* ثابت vtxp = جديد Reorder_Logic_راس{m_graphp, currp};
            currp->user3p(vtxp);

            // Visit the statement - this can recursively reorder sub statements
            m_stmtStackps.ڈالو_پٹھ(vtxp);
            iterate(currp);
            m_stmtStackps.نکالو_پٹھ();
        }

        إذا (m_noReorderWhy) {  // Jump or something nasty
            UINFO(9, "  NoReorderBlock because " << m_noReorderWhy);
            أرجع;
        }

        // Reorder statements in this block
        cleanup_بلاک_گراف(nodep);
        reorder_بلاک(nodep);

        // 'nodep' might no longer be the head of the list, rewind
        طالما (nodep->backp()->nextp() == nodep) nodep = nodep->backp();

        // Delete vertexes and edges only applying to this block
        لكل (Ast_گرہ* currp = nodep; currp; currp = currp->nextp()) {
            currp->user3u().منزل<Reorder_Logic_راس*>()->unlinkDelete(m_graphp);
            currp->user3p(خالی_سوچک);
        }
    }

    // VISITORS
    فراغ ویکھو(AstAlways* nodep) override {
        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_وچ_Dly);
        VL_RESTORER(m_noReorderWhy);

        V3_گراف گراف;
        m_graphp = &گراف;
        m_impureVtxp = خالی_سوچک;
        m_وچ_Dly = خطأ;
        m_noReorderWhy = خالی_سوچک;

        ثابت VNUser1_وچ_Use user1_وچ_Use;
        ثابت VNUser2_وچ_Use user2_وچ_Use;
        ثابت VNUser3_وچ_Use user3_وچ_Use;
        ثابت VNUser4_وچ_Use user4_وچ_Use;

        UINFOTREE(9, nodep, "", "alwIn:");
        process_بلاک(nodep->stmtsp());
        UINFOTREE(9, nodep, "", "alwOut");

        m_stmtStackps.clear();
    }

    فراغ ویکھو(Ast_گرہ_If* nodep) override {
        إذا (!m_graphp || m_noReorderWhy) أرجع;
        iterate_And_اگلا_Null(nodep->condp());
        process_بلاک(nodep->thensp());
        process_بلاک(nodep->elsesp());
    }

    فراغ ویکھو(AstJumpGo* nodep) override {
        إذا (!m_graphp || m_noReorderWhy) أرجع;
        m_noReorderWhy = "JumpGo";
        iterateChildren(nodep);
    }

    فراغ ویکھو(Ast_گرہ_CCall* nodep) override {
        إذا (!m_graphp || m_noReorderWhy) أرجع;
        // The callee is not inlined, so the variables it reads and writes carry no VarRef at
        // the call site, and the scoreboard would let assignments to them move across the
        // call. Treat any such call as a barrier.
        m_noReorderWhy = "CCall";
        iterateChildren(nodep);
    }

    فراغ ویکھو(Ast_اظہاریہ_Stmt* nodep) override {
        إذا (!m_graphp || m_noReorderWhy) أرجع;
        VL_RESTORER(m_وچ_Dly);
        m_وچ_Dly = خطأ;
        iterateChildren(nodep);
    }

    فراغ ویکھو(Ast_سونپو_Dly* nodep) override {
        إذا (!m_graphp || m_noReorderWhy) أرجع;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        إذا (Ast_گرہ* ثابت controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_وچ_Dly);
        m_وچ_Dly = سچ;
        iterate(nodep->lhsp());
    }

    فراغ ویکھو(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        إذا (m_graphp) m_noReorderWhy = "Handle";
        ویکھو(static_cast<Ast_گرہ*>(nodep));
    }

    فراغ ویکھو(AstVarRef* nodep) override {
        إذا (!m_graphp || m_noReorderWhy) أرجع;
        إذا (m_stmtStackps.خالی_تھاواں()) أرجع;

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
        إذا (!m_وچ_Dly) {
            لكل (Reorder_Logic_راس* ثابت vtxp : m_stmtStackps) {
                جديد ReorderLVEdge{m_graphp, vstdp, vtxp};
            }
            أرجع;
        }

        // Variable is written by NBA
        إذا (!vscp->user2p()) {
            Reorder_Var_Post_راس* ثابت vpostp = جديد Reorder_Var_Post_راس{m_graphp, vscp};
            vscp->user2p(vpostp);
            جديد Reorder_Post_کنارا{m_graphp, vstdp, vpostp};
        }
        Reorder_Var_Post_راس* ثابت vpostp = vscp->user2u().منزل<Reorder_Var_Post_راس*>();
        لكل (Reorder_Logic_راس* ثابت vtxp : m_stmtStackps) {
            جديد ReorderLVEdge{m_graphp, vpostp, vtxp};
        }
    }

    فراغ ویکھو(Ast_گرہ* nodep) override {
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
                جديد Reorder_Scorebd_کنارا{m_graphp, m_impureVtxp, vtxp};
            }
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
عام:
    واضح ReorderVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReorderVisitor() override = افتراضي;
};

}  // namespace

//######################################################################
// V3Reorder class functions

فراغ V3Reorder::reorderAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReorderVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("reorder", 0, dump_درخت_Either_Level() >= 3);
}
