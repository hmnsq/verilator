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

نامخلا {

//######################################################################
// Support classes

جماعت Reorder_گرہ_راس VL_NOT_FINAL : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Reorder_گرہ_راس, V3_گراف_راس)
    Ast_گرہ* ثابت m_nodep;

محفوظ:
    Reorder_گرہ_راس(V3_گراف* graphp, Ast_گرہ* nodep)
        : V3_گراف_راس{graphp}
        , m_nodep{nodep} {}
    ~Reorder_گرہ_راس() override = طےشدہ;
    // ACCESSORS
    // Do not make accessor for nodep(),  It may change due to
    // reordering a lower block, but we don't repair it
    std::string name() ثابت override {
        std::string str = cvt_مقصد_Hex(m_nodep) + '\n';
        اگر (AstVarScope* ثابت vscp = VN_CAST(m_nodep, VarScope)) {
            str += vscp->prettyName();
        } ورنہ {
            str += V3EmitV::debug_Verilog_For_درخت(m_nodep);
            str = VString::quoteBackslash(str);
            str = VString::quoteAny(str, '"', '\\');
            str = VString::replaceSubstr(str, "\n", "\\l");
        }
        واپس str;
    }
    File_سطر* fileline() ثابت override { واپس nodep()->fileline(); }
    std::string dotShape() ثابت override { واپس VN_IS(m_nodep, VarScope) ? "ellipse" : "box"; }

عوامی:
    مجازی Ast_گرہ* nodep() ثابت { واپس m_nodep; }
};

جماعت Reorder_Impure_راس final : عوامی Reorder_گرہ_راس {
    VL_RTTI_IMPL(Reorder_Impure_راس, Reorder_گرہ_راس)
عوامی:
    صریح Reorder_Impure_راس(V3_گراف* graphp, Ast_گرہ* nodep)
        : Reorder_گرہ_راس{graphp, nodep} {}
    ~Reorder_Impure_راس() override = طےشدہ;
    string name() ثابت override VL_MT_STABLE { واپس "*IMPURE*"; }
    string dotColor() ثابت override { واپس "red"; }
};

جماعت Reorder_Logic_راس final : عوامی Reorder_گرہ_راس {
    VL_RTTI_IMPL(Reorder_Logic_راس, Reorder_گرہ_راس)
عوامی:
    Reorder_Logic_راس(V3_گراف* graphp, Ast_گرہ* nodep)
        : Reorder_گرہ_راس{graphp, nodep} {}
    ~Reorder_Logic_راس() override = طےشدہ;
    string dotColor() ثابت override { واپس "black"; }
};

جماعت Reorder_Var_Std_راس final : عوامی Reorder_گرہ_راس {
    VL_RTTI_IMPL(Reorder_Var_Std_راس, Reorder_گرہ_راس)
عوامی:
    Reorder_Var_Std_راس(V3_گراف* graphp, AstVarScope* nodep)
        : Reorder_گرہ_راس{graphp, nodep} {}
    ~Reorder_Var_Std_راس() override = طےشدہ;
    string dotColor() ثابت override { واپس "blue"; }
};

جماعت Reorder_Var_Post_راس final : عوامی Reorder_گرہ_راس {
    VL_RTTI_IMPL(Reorder_Var_Post_راس, Reorder_گرہ_راس)
عوامی:
    Reorder_Var_Post_راس(V3_گراف* graphp, AstVarScope* nodep)
        : Reorder_گرہ_راس{graphp, nodep} {}
    ~Reorder_Var_Post_راس() override = طےشدہ;
    string name() ثابت override { واپس "POST "s + Reorder_گرہ_راس::name(); }
    string dotColor() ثابت override { واپس "green"; }
};

//######################################################################
// Edge types

جماعت Reorder_کنارہ VL_NOT_FINAL : عوامی V3_گراف_کنارہ {
    VL_RTTI_IMPL(Reorder_کنارہ, V3_گراف_کنارہ)
    uint32_t m_ignore_اندر_قدم = 0;  // Step number that if set to, causes this edge to be ignored
    ساکن uint32_t s_قدم_عدد;  // Global step number
محفوظ:
    ساکن constexpr صحیح وزن_معمول = 10;
    Reorder_کنارہ(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی, منطقی cutable)
        : V3_گراف_کنارہ{graphp, fromp, چوٹی, وزن_معمول, cutable} {}
    ~Reorder_کنارہ() override = طےشدہ;

    مجازی منطقی followScoreboard() ثابت = 0;

    std::string dotStyle() ثابت override {
        واپس ignore_This_قدم() ? "dotted" : V3_گراف_کنارہ::dotStyle();
    }

عوامی:
    // Iterator for graph functions
    ساکن خالی increment_قدم() { ++s_قدم_عدد; }
    منطقی ignore_This_قدم() ثابت { واپس m_ignore_اندر_قدم == s_قدم_عدد; }
    خالی سیٹ_Ignore_This_قدم() { m_ignore_اندر_قدم = s_قدم_عدد; }

    ساکن منطقی followScoreboard(ثابت V3_گراف_کنارہ* edgep) {
        ثابت Reorder_کنارہ& کنارہ = *edgep->as<Reorder_کنارہ>();
        واپس !کنارہ.ignore_This_قدم() && کنارہ.followScoreboard();
    }
    ساکن منطقی followCyclic(ثابت V3_گراف_کنارہ* edgep) {
        ثابت Reorder_کنارہ& کنارہ = *edgep->as<Reorder_کنارہ>();
        واپس !کنارہ.ignore_This_قدم();
    }
};
uint32_t Reorder_کنارہ::s_قدم_عدد = 0;

جماعت Reorder_Post_کنارہ final : عوامی Reorder_کنارہ {
    VL_RTTI_IMPL(Reorder_Post_کنارہ, Reorder_کنارہ)
عوامی:
    Reorder_Post_کنارہ(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی)
        : Reorder_کنارہ{graphp, fromp, چوٹی, CUTABLE} {}
    ~Reorder_Post_کنارہ() override = طےشدہ;
    منطقی followScoreboard() ثابت override { واپس جھوٹ; }
    string dotColor() ثابت override { واپس "khaki"; }
};

جماعت ReorderLVEdge final : عوامی Reorder_کنارہ {
    VL_RTTI_IMPL(ReorderLVEdge, Reorder_کنارہ)
عوامی:
    ReorderLVEdge(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی)
        : Reorder_کنارہ{graphp, fromp, چوٹی, CUTABLE} {}
    ~ReorderLVEdge() override = طےشدہ;
    منطقی followScoreboard() ثابت override { واپس سچ; }
    string dotColor() ثابت override { واپس "yellowGreen"; }
};

جماعت ReorderRVEdge final : عوامی Reorder_کنارہ {
    VL_RTTI_IMPL(ReorderRVEdge, Reorder_کنارہ)
عوامی:
    ReorderRVEdge(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی)
        : Reorder_کنارہ{graphp, fromp, چوٹی, CUTABLE} {}
    ~ReorderRVEdge() override = طےشدہ;
    منطقی followScoreboard() ثابت override { واپس سچ; }
    string dotColor() ثابت override { واپس "green"; }
};

جماعت Reorder_Scorebd_کنارہ final : عوامی Reorder_کنارہ {
    VL_RTTI_IMPL(Reorder_Scorebd_کنارہ, Reorder_کنارہ)
عوامی:
    Reorder_Scorebd_کنارہ(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی)
        : Reorder_کنارہ{graphp, fromp, چوٹی, CUTABLE} {}
    ~Reorder_Scorebd_کنارہ() override = طےشدہ;
    منطقی followScoreboard() ثابت override { واپس سچ; }
    string dotColor() ثابت override { واپس "blue"; }
};

جماعت Reorder_Strict_کنارہ final : عوامی Reorder_کنارہ {
    VL_RTTI_IMPL(Reorder_Strict_کنارہ, Reorder_کنارہ)
    // A strict order, based on the original statement order in the graph
    // The only non-cutable edge type
عوامی:
    Reorder_Strict_کنارہ(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی)
        : Reorder_کنارہ{graphp, fromp, چوٹی, NOT_CUTABLE} {}
    ~Reorder_Strict_کنارہ() override = طےشدہ;
    منطقی followScoreboard() ثابت override { واپس سچ; }
    string dotColor() ثابت override { واپس "blue"; }
};

//######################################################################
// Reorder class functions

جماعت ReorderVisitor final : عوامی VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> ReorderVarStdVertex*.  Usage var, 0=not set yet
    // AstVarScope::user2p  -> ReorderVarPostVertex*.  Delayed assignment var, 0=not set yet
    // Ast*::user3p         -> ReorderLogicVertex*.  Statement (temporary only)
    // Ast*::user4          -> int.  Current ordering number (reorderBlock usage)

    // STATE
    V3_گراف* m_graphp = خالی_اشاریہ;  // Scoreboard of var usages/dependencies
    Reorder_Impure_راس* m_impureVtxp = خالی_اشاریہ;  // Element specifying PLI ordering
    منطقی m_اندر_Dly = جھوٹ;  // Inside ASSIGNDLY
    ثابت حرف* m_noReorderWhy = خالی_اشاریہ;  // Reason we can't reorder
    std::سمتیہ<Reorder_Logic_راس*> m_stmtStackps;  // Current statements being tracked

    // METHODS
    خالی cleanup_بلاک_گراف(Ast_گرہ* nodep) {
        // Transform the graph into what we need
        UINFO(5, "ReorderBlock " << nodep);

        // Simplify graph by removing redundant edges
        m_graphp->remove_Redundant_کنارے_Max(&V3_گراف_کنارہ::followAlwaysTrue);
        اگر (dump_گراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_nodup", جھوٹ);

        // Mark all the logic for this step by setting Vertex::user() to true
        m_graphp->userClearVertices();
        برائے (Ast_گرہ* nextp = nodep; nextp; nextp = nextp->nextp()) {
            nextp->user3u().مقصد<Reorder_Logic_راس*>()->user(سچ);
        }

        // New step
        Reorder_کنارہ::increment_قدم();

        // If a var vertex has only inputs, it's a input-only node,
        // and can be ignored for coloring **this block only**
        برائے (V3_گراف_راس& vtx : m_graphp->vertices()) {
            اگر (!vtx.برآمد_خالی_خانے()) جاری;
            اگر (!vtx.is<Reorder_Var_Std_راس>()) جاری;
            برائے (V3_گراف_کنارہ& کنارہ : vtx.اندر_کنارے()) کنارہ.as<Reorder_کنارہ>()->سیٹ_Ignore_This_قدم();
        }

        // For reordering this single block only, mark all logic
        // vertexes not involved with this step as unimportant
        برائے (V3_گراف_راس& راس : m_graphp->vertices()) {
            اگر (راس.user()) جاری;
            اگر (!راس.is<Reorder_Logic_راس>()) جاری;
            برائے (V3_گراف_کنارہ& کنارہ : راس.اندر_کنارے()) کنارہ.as<Reorder_کنارہ>()->سیٹ_Ignore_This_قدم();
            برائے (V3_گراف_کنارہ& کنارہ : راس.برآمد_کنارے())
                کنارہ.as<Reorder_کنارہ>()->سیٹ_Ignore_This_قدم();
        }

        // Weak coloring to determine what needs to remain in order
        // This follows all step-relevant edges excluding PostEdges, which are done later
        m_graphp->weaklyConnected(&Reorder_کنارہ::followScoreboard);

        // Add hard orderings between all nodes of same color, in the order they appeared
        std::unordered_map<uint32_t, Reorder_Logic_راس*> آخری_کا_Color;
        برائے (Ast_گرہ* currp = nodep; currp; currp = currp->nextp()) {
            Reorder_Logic_راس* ثابت vtxp = currp->user3u().مقصد<Reorder_Logic_راس*>();
            ثابت uint32_t color = vtxp->color();
            اگر (آخری_کا_Color[color]) نیا Reorder_Strict_کنارہ{m_graphp, آخری_کا_Color[color], vtxp};
            آخری_کا_Color[color] = vtxp;
        }

        // And a real ordering to get the statements into something reasonable
        // We don't care if there's cutable violations here...
        // Non-cutable violations should be impossible; as those edges are program-order
        اگر (dump_گراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_pre", جھوٹ);
        m_graphp->acyclic(&Reorder_کنارہ::followCyclic);
        m_graphp->rank(&Reorder_کنارہ::followCyclic);  // Or order(), but that's more expensive
        اگر (dump_گراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_opt", جھوٹ);
    }

    خالی reorder_بلاک(Ast_گرہ* nodep) {
        // Reorder statements in the completed graph

        // Map the rank numbers into nodes they associate with
        std::multimap<uint32_t, Ast_گرہ*> rankMap;
        صحیح curr_ترتیب = 0;  // Existing sequence number of assignment
        برائے (Ast_گرہ* currp = nodep; currp; currp = currp->nextp()) {
            ثابت Reorder_Logic_راس* ثابت vtxp = currp->user3u().مقصد<Reorder_Logic_راس*>();
            rankMap.emplace(vtxp->rank(), currp);
            currp->user4(++curr_ترتیب);  // Record current ordering
        }

        // Is the current ordering OK?
        منطقی leaveAlone = سچ;
        uint64_t new_ترتیب = 0;  // New sequence number of assignment
        برائے (ثابت خودکار& شے_3 : rankMap) {
            ثابت Ast_گرہ* ثابت nextp = شے_3.دوسرا;
            اگر (++new_ترتیب != nextp->user4()) leaveAlone = جھوٹ;
        }
        اگر (leaveAlone) {
            UINFO(6, "   No changes");
            واپس;
        }

        VNRelinker replaceHandle;  // Where to add the list
        Ast_گرہ* newListp = خالی_اشاریہ;
        برائے (ثابت خودکار& شے_3 : rankMap) {
            Ast_گرہ* ثابت nextp = شے_3.دوسرا;
            UINFO(6, "   New order: " << nextp);
            nextp->unlink_Fr_عقب(nextp == nodep ? &replaceHandle : خالی_اشاریہ);
            newListp = Ast_گرہ::جوڑو_اگلا(newListp, nextp);
        }
        replaceHandle.relink(newListp);
    }

    خالی process_بلاک(Ast_گرہ* nodep) {
        اگر (m_noReorderWhy) واپس;

        // Empty lists are ignorable
        اگر (!nodep) واپس;
        UASSERT_OBJ(nodep->پہلا_Abovep(), nodep, "Node passed is in not head of list");
        UASSERT_OBJ(!nodep->user3p(), nodep, "Should not have a logic vertex");

        // It nothing to reorder with, just iterate
        اگر (!nodep->nextp()) {
            iterate(nodep);
            واپس;
        }

        // Process it
        UINFO(9, "  processBlock " << nodep);

        // Iterate across current block, making the scoreboard
        برائے (Ast_گرہ* currp = nodep; currp; currp = currp->nextp()) {
            // Create the logic vertex for this statement
            UASSERT_OBJ(!currp->user3p(), currp, "user3p should not be set");
            Reorder_Logic_راس* ثابت vtxp = نیا Reorder_Logic_راس{m_graphp, currp};
            currp->user3p(vtxp);

            // Visit the statement - this can recursively reorder sub statements
            m_stmtStackps.ڈالو_عقب(vtxp);
            iterate(currp);
            m_stmtStackps.نکالو_عقب();
        }

        اگر (m_noReorderWhy) {  // Jump or something nasty
            UINFO(9, "  NoReorderBlock because " << m_noReorderWhy);
            واپس;
        }

        // Reorder statements in this block
        cleanup_بلاک_گراف(nodep);
        reorder_بلاک(nodep);

        // 'nodep' might no longer be the head of the list, rewind
        جب (nodep->backp()->nextp() == nodep) nodep = nodep->backp();

        // Delete vertexes and edges only applying to this block
        برائے (Ast_گرہ* currp = nodep; currp; currp = currp->nextp()) {
            currp->user3u().مقصد<Reorder_Logic_راس*>()->unlinkDelete(m_graphp);
            currp->user3p(خالی_اشاریہ);
        }
    }

    // VISITORS
    خالی دورہ(AstAlways* nodep) override {
        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_اندر_Dly);
        VL_RESTORER(m_noReorderWhy);

        V3_گراف گراف;
        m_graphp = &گراف;
        m_impureVtxp = خالی_اشاریہ;
        m_اندر_Dly = جھوٹ;
        m_noReorderWhy = خالی_اشاریہ;

        ثابت VNUser1_اندر_Use user1_اندر_Use;
        ثابت VNUser2_اندر_Use user2_اندر_Use;
        ثابت VNUser3_اندر_Use user3_اندر_Use;
        ثابت VNUser4_اندر_Use user4_اندر_Use;

        UINFOTREE(9, nodep, "", "alwIn:");
        process_بلاک(nodep->stmtsp());
        UINFOTREE(9, nodep, "", "alwOut");

        m_stmtStackps.clear();
    }

    خالی دورہ(Ast_گرہ_If* nodep) override {
        اگر (!m_graphp || m_noReorderWhy) واپس;
        iterate_And_اگلا_Null(nodep->condp());
        process_بلاک(nodep->thensp());
        process_بلاک(nodep->elsesp());
    }

    خالی دورہ(AstJumpGo* nodep) override {
        اگر (!m_graphp || m_noReorderWhy) واپس;
        m_noReorderWhy = "JumpGo";
        iterateChildren(nodep);
    }

    خالی دورہ(Ast_گرہ_CCall* nodep) override {
        اگر (!m_graphp || m_noReorderWhy) واپس;
        // The callee is not inlined, so the variables it reads and writes carry no VarRef at
        // the call site, and the scoreboard would let assignments to them move across the
        // call. Treat any such call as a barrier.
        m_noReorderWhy = "CCall";
        iterateChildren(nodep);
    }

    خالی دورہ(Ast_اظہاریہ_Stmt* nodep) override {
        اگر (!m_graphp || m_noReorderWhy) واپس;
        VL_RESTORER(m_اندر_Dly);
        m_اندر_Dly = جھوٹ;
        iterateChildren(nodep);
    }

    خالی دورہ(Ast_تفویض_Dly* nodep) override {
        اگر (!m_graphp || m_noReorderWhy) واپس;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        اگر (Ast_گرہ* ثابت controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_اندر_Dly);
        m_اندر_Dly = سچ;
        iterate(nodep->lhsp());
    }

    خالی دورہ(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        اگر (m_graphp) m_noReorderWhy = "Handle";
        دورہ(static_cast<Ast_گرہ*>(nodep));
    }

    خالی دورہ(AstVarRef* nodep) override {
        اگر (!m_graphp || m_noReorderWhy) واپس;
        اگر (m_stmtStackps.خالی_خانے()) واپس;

        // Reads of constants can be ignored - TODO: This should be "constexpr", not run-time const
        اگر (nodep->varp()->isConst()) واپس;

        // SPEEDUP: We add duplicate edges, that should be fixed

        AstVarScope* ثابت vscp = nodep->varScopep();

        // Create vertexes for variable
        اگر (!vscp->user1p()) vscp->user1p(نیا Reorder_Var_Std_راس{m_graphp, vscp});
        Reorder_Var_Std_راس* ثابت vstdp = vscp->user1u().مقصد<Reorder_Var_Std_راس*>();

        // Variable is read
        اگر (nodep->access().isReadOnly()) {
            برائے (Reorder_Logic_راس* ثابت vtxp : m_stmtStackps) {
                نیا ReorderRVEdge{m_graphp, vtxp, vstdp};
            }
            واپس;
        }

        // Variable is written, not NBA
        اگر (!m_اندر_Dly) {
            برائے (Reorder_Logic_راس* ثابت vtxp : m_stmtStackps) {
                نیا ReorderLVEdge{m_graphp, vstdp, vtxp};
            }
            واپس;
        }

        // Variable is written by NBA
        اگر (!vscp->user2p()) {
            Reorder_Var_Post_راس* ثابت vpostp = نیا Reorder_Var_Post_راس{m_graphp, vscp};
            vscp->user2p(vpostp);
            نیا Reorder_Post_کنارہ{m_graphp, vstdp, vpostp};
        }
        Reorder_Var_Post_راس* ثابت vpostp = vscp->user2u().مقصد<Reorder_Var_Post_راس*>();
        برائے (Reorder_Logic_راس* ثابت vtxp : m_stmtStackps) {
            نیا ReorderLVEdge{m_graphp, vpostp, vtxp};
        }
    }

    خالی دورہ(Ast_گرہ* nodep) override {
        // Outside AstAlways, just descend
        اگر (!m_graphp) {
            iterateChildren(nodep);
            واپس;
        }

        // Early exit if decided not to reorder
        اگر (m_noReorderWhy) واپس;

        // Timing control prevents reordering
        اگر (nodep->isTimingControl()) {
            m_noReorderWhy = "TimingControl";
            واپس;
        }

        // Order all impure statements with other impure statements
        اگر (!nodep->is_خالص()) {
            اگر (!m_impureVtxp) m_impureVtxp = نیا Reorder_Impure_راس{m_graphp, nodep};
            // This edge is only used to find weakly connected components, so one edge is enough
            برائے (Reorder_Logic_راس* ثابت vtxp : m_stmtStackps) {
                نیا Reorder_Scorebd_کنارہ{m_graphp, m_impureVtxp, vtxp};
            }
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
عوامی:
    صریح ReorderVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReorderVisitor() override = طےشدہ;
};

}  // namespace

//######################################################################
// V3Reorder class functions

خالی V3Reorder::reorderAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReorderVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("reorder", 0, dump_درخت_Either_Level() >= 3);
}
