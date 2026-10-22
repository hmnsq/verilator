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

صنف Reorder_گره_رأس VL_NOT_FINAL : عام V3_گراف_رأس {
    VL_RTTI_IMPL(Reorder_گره_رأس, V3_گراف_رأس)
    Ast_گره* ثابت m_nodep;

محمي:
    Reorder_گره_رأس(V3_گراف* graphp, Ast_گره* nodep)
        : V3_گراف_رأس{graphp}
        , m_nodep{nodep} {}
    ~Reorder_گره_رأس() override = افتراضي;
    // ACCESSORS
    // Do not make accessor for nodep(),  It may change due to
    // reordering a lower block, but we don't repair it
    std::string name() ثابت override {
        std::string str = cvt_مقصد_Hex(m_nodep) + '\n';
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
    مجازی Ast_گره* nodep() ثابت { أرجع m_nodep; }
};

صنف Reorder_Impure_رأس final : عام Reorder_گره_رأس {
    VL_RTTI_IMPL(Reorder_Impure_رأس, Reorder_گره_رأس)
عام:
    صریح Reorder_Impure_رأس(V3_گراف* graphp, Ast_گره* nodep)
        : Reorder_گره_رأس{graphp, nodep} {}
    ~Reorder_Impure_رأس() override = افتراضي;
    string name() ثابت override VL_MT_STABLE { أرجع "*IMPURE*"; }
    string dotColor() ثابت override { أرجع "red"; }
};

صنف Reorder_Logic_رأس final : عام Reorder_گره_رأس {
    VL_RTTI_IMPL(Reorder_Logic_رأس, Reorder_گره_رأس)
عام:
    Reorder_Logic_رأس(V3_گراف* graphp, Ast_گره* nodep)
        : Reorder_گره_رأس{graphp, nodep} {}
    ~Reorder_Logic_رأس() override = افتراضي;
    string dotColor() ثابت override { أرجع "black"; }
};

صنف Reorder_Var_Std_رأس final : عام Reorder_گره_رأس {
    VL_RTTI_IMPL(Reorder_Var_Std_رأس, Reorder_گره_رأس)
عام:
    Reorder_Var_Std_رأس(V3_گراف* graphp, AstVarScope* nodep)
        : Reorder_گره_رأس{graphp, nodep} {}
    ~Reorder_Var_Std_رأس() override = افتراضي;
    string dotColor() ثابت override { أرجع "blue"; }
};

صنف Reorder_Var_Post_رأس final : عام Reorder_گره_رأس {
    VL_RTTI_IMPL(Reorder_Var_Post_رأس, Reorder_گره_رأس)
عام:
    Reorder_Var_Post_رأس(V3_گراف* graphp, AstVarScope* nodep)
        : Reorder_گره_رأس{graphp, nodep} {}
    ~Reorder_Var_Post_رأس() override = افتراضي;
    string name() ثابت override { أرجع "POST "s + Reorder_گره_رأس::name(); }
    string dotColor() ثابت override { أرجع "green"; }
};

//######################################################################
// Edge types

صنف Reorder_یال VL_NOT_FINAL : عام V3_گراف_یال {
    VL_RTTI_IMPL(Reorder_یال, V3_گراف_یال)
    uint32_t m_ignore_درون_گام = 0;  // Step number that if set to, causes this edge to be ignored
    ساكن uint32_t s_گام_رقم;  // Global step number
محمي:
    ساكن constexpr صحيح وزن_نرمال = 10;
    Reorder_یال(V3_گراف* graphp, V3_گراف_رأس* fromp, V3_گراف_رأس* بالاترین, منطقي cutable)
        : V3_گراف_یال{graphp, fromp, بالاترین, وزن_نرمال, cutable} {}
    ~Reorder_یال() override = افتراضي;

    مجازی منطقي followScoreboard() ثابت = 0;

    std::string dotStyle() ثابت override {
        أرجع ignore_This_گام() ? "dotted" : V3_گراف_یال::dotStyle();
    }

عام:
    // Iterator for graph functions
    ساكن فراغ increment_گام() { ++s_گام_رقم; }
    منطقي ignore_This_گام() ثابت { أرجع m_ignore_درون_گام == s_گام_رقم; }
    فراغ مجموعه_Ignore_This_گام() { m_ignore_درون_گام = s_گام_رقم; }

    ساكن منطقي followScoreboard(ثابت V3_گراف_یال* edgep) {
        ثابت Reorder_یال& یال = *edgep->as<Reorder_یال>();
        أرجع !یال.ignore_This_گام() && یال.followScoreboard();
    }
    ساكن منطقي followCyclic(ثابت V3_گراف_یال* edgep) {
        ثابت Reorder_یال& یال = *edgep->as<Reorder_یال>();
        أرجع !یال.ignore_This_گام();
    }
};
uint32_t Reorder_یال::s_گام_رقم = 0;

صنف Reorder_Post_یال final : عام Reorder_یال {
    VL_RTTI_IMPL(Reorder_Post_یال, Reorder_یال)
عام:
    Reorder_Post_یال(V3_گراف* graphp, V3_گراف_رأس* fromp, V3_گراف_رأس* بالاترین)
        : Reorder_یال{graphp, fromp, بالاترین, CUTABLE} {}
    ~Reorder_Post_یال() override = افتراضي;
    منطقي followScoreboard() ثابت override { أرجع خطأ; }
    string dotColor() ثابت override { أرجع "khaki"; }
};

صنف ReorderLVEdge final : عام Reorder_یال {
    VL_RTTI_IMPL(ReorderLVEdge, Reorder_یال)
عام:
    ReorderLVEdge(V3_گراف* graphp, V3_گراف_رأس* fromp, V3_گراف_رأس* بالاترین)
        : Reorder_یال{graphp, fromp, بالاترین, CUTABLE} {}
    ~ReorderLVEdge() override = افتراضي;
    منطقي followScoreboard() ثابت override { أرجع درست; }
    string dotColor() ثابت override { أرجع "yellowGreen"; }
};

صنف ReorderRVEdge final : عام Reorder_یال {
    VL_RTTI_IMPL(ReorderRVEdge, Reorder_یال)
عام:
    ReorderRVEdge(V3_گراف* graphp, V3_گراف_رأس* fromp, V3_گراف_رأس* بالاترین)
        : Reorder_یال{graphp, fromp, بالاترین, CUTABLE} {}
    ~ReorderRVEdge() override = افتراضي;
    منطقي followScoreboard() ثابت override { أرجع درست; }
    string dotColor() ثابت override { أرجع "green"; }
};

صنف Reorder_Scorebd_یال final : عام Reorder_یال {
    VL_RTTI_IMPL(Reorder_Scorebd_یال, Reorder_یال)
عام:
    Reorder_Scorebd_یال(V3_گراف* graphp, V3_گراف_رأس* fromp, V3_گراف_رأس* بالاترین)
        : Reorder_یال{graphp, fromp, بالاترین, CUTABLE} {}
    ~Reorder_Scorebd_یال() override = افتراضي;
    منطقي followScoreboard() ثابت override { أرجع درست; }
    string dotColor() ثابت override { أرجع "blue"; }
};

صنف Reorder_Strict_یال final : عام Reorder_یال {
    VL_RTTI_IMPL(Reorder_Strict_یال, Reorder_یال)
    // A strict order, based on the original statement order in the graph
    // The only non-cutable edge type
عام:
    Reorder_Strict_یال(V3_گراف* graphp, V3_گراف_رأس* fromp, V3_گراف_رأس* بالاترین)
        : Reorder_یال{graphp, fromp, بالاترین, NOT_CUTABLE} {}
    ~Reorder_Strict_یال() override = افتراضي;
    منطقي followScoreboard() ثابت override { أرجع درست; }
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
    V3_گراف* m_graphp = اشاره_تهی;  // Scoreboard of var usages/dependencies
    Reorder_Impure_رأس* m_impureVtxp = اشاره_تهی;  // Element specifying PLI ordering
    منطقي m_درون_Dly = خطأ;  // Inside ASSIGNDLY
    ثابت حرف* m_noReorderWhy = اشاره_تهی;  // Reason we can't reorder
    std::وکتور<Reorder_Logic_رأس*> m_stmtStackps;  // Current statements being tracked

    // METHODS
    فراغ cleanup_بلوک_گراف(Ast_گره* nodep) {
        // Transform the graph into what we need
        UINFO(5, "ReorderBlock " << nodep);

        // Simplify graph by removing redundant edges
        m_graphp->remove_Redundant_یالها_Max(&V3_گراف_یال::followAlwaysTrue);
        إذا (dump_گراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_nodup", خطأ);

        // Mark all the logic for this step by setting Vertex::user() to true
        m_graphp->userClearVertices();
        لكل (Ast_گره* nextp = nodep; nextp; nextp = nextp->nextp()) {
            nextp->user3u().مقصد<Reorder_Logic_رأس*>()->user(درست);
        }

        // New step
        Reorder_یال::increment_گام();

        // If a var vertex has only inputs, it's a input-only node,
        // and can be ignored for coloring **this block only**
        لكل (V3_گراف_رأس& vtx : m_graphp->vertices()) {
            إذا (!vtx.بیرون_خالی()) استمر;
            إذا (!vtx.is<Reorder_Var_Std_رأس>()) استمر;
            لكل (V3_گراف_یال& یال : vtx.درون_یالها()) یال.as<Reorder_یال>()->مجموعه_Ignore_This_گام();
        }

        // For reordering this single block only, mark all logic
        // vertexes not involved with this step as unimportant
        لكل (V3_گراف_رأس& رأس : m_graphp->vertices()) {
            إذا (رأس.user()) استمر;
            إذا (!رأس.is<Reorder_Logic_رأس>()) استمر;
            لكل (V3_گراف_یال& یال : رأس.درون_یالها()) یال.as<Reorder_یال>()->مجموعه_Ignore_This_گام();
            لكل (V3_گراف_یال& یال : رأس.بیرون_یالها())
                یال.as<Reorder_یال>()->مجموعه_Ignore_This_گام();
        }

        // Weak coloring to determine what needs to remain in order
        // This follows all step-relevant edges excluding PostEdges, which are done later
        m_graphp->weaklyConnected(&Reorder_یال::followScoreboard);

        // Add hard orderings between all nodes of same color, in the order they appeared
        std::unordered_map<uint32_t, Reorder_Logic_رأس*> آخرین_از_Color;
        لكل (Ast_گره* currp = nodep; currp; currp = currp->nextp()) {
            Reorder_Logic_رأس* ثابت vtxp = currp->user3u().مقصد<Reorder_Logic_رأس*>();
            ثابت uint32_t color = vtxp->color();
            إذا (آخرین_از_Color[color]) جديد Reorder_Strict_یال{m_graphp, آخرین_از_Color[color], vtxp};
            آخرین_از_Color[color] = vtxp;
        }

        // And a real ordering to get the statements into something reasonable
        // We don't care if there's cutable violations here...
        // Non-cutable violations should be impossible; as those edges are program-order
        إذا (dump_گراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_pre", خطأ);
        m_graphp->acyclic(&Reorder_یال::followCyclic);
        m_graphp->rank(&Reorder_یال::followCyclic);  // Or order(), but that's more expensive
        إذا (dump_گراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_opt", خطأ);
    }

    فراغ reorder_بلوک(Ast_گره* nodep) {
        // Reorder statements in the completed graph

        // Map the rank numbers into nodes they associate with
        std::multimap<uint32_t, Ast_گره*> rankMap;
        صحيح curr_ترتیب = 0;  // Existing sequence number of assignment
        لكل (Ast_گره* currp = nodep; currp; currp = currp->nextp()) {
            ثابت Reorder_Logic_رأس* ثابت vtxp = currp->user3u().مقصد<Reorder_Logic_رأس*>();
            rankMap.emplace(vtxp->rank(), currp);
            currp->user4(++curr_ترتیب);  // Record current ordering
        }

        // Is the current ordering OK?
        منطقي leaveAlone = درست;
        uint64_t new_ترتیب = 0;  // New sequence number of assignment
        لكل (ثابت خودکار& قلم_2 : rankMap) {
            ثابت Ast_گره* ثابت nextp = قلم_2.دومی;
            إذا (++new_ترتیب != nextp->user4()) leaveAlone = خطأ;
        }
        إذا (leaveAlone) {
            UINFO(6, "   No changes");
            أرجع;
        }

        VNRelinker replaceHandle;  // Where to add the list
        Ast_گره* newListp = اشاره_تهی;
        لكل (ثابت خودکار& قلم_2 : rankMap) {
            Ast_گره* ثابت nextp = قلم_2.دومی;
            UINFO(6, "   New order: " << nextp);
            nextp->unlink_Fr_پشت(nextp == nodep ? &replaceHandle : اشاره_تهی);
            newListp = Ast_گره::جمع_کن_بعدی(newListp, nextp);
        }
        replaceHandle.relink(newListp);
    }

    فراغ process_بلوک(Ast_گره* nodep) {
        إذا (m_noReorderWhy) أرجع;

        // Empty lists are ignorable
        إذا (!nodep) أرجع;
        UASSERT_OBJ(nodep->اولی_Abovep(), nodep, "Node passed is in not head of list");
        UASSERT_OBJ(!nodep->user3p(), nodep, "Should not have a logic vertex");

        // It nothing to reorder with, just iterate
        إذا (!nodep->nextp()) {
            iterate(nodep);
            أرجع;
        }

        // Process it
        UINFO(9, "  processBlock " << nodep);

        // Iterate across current block, making the scoreboard
        لكل (Ast_گره* currp = nodep; currp; currp = currp->nextp()) {
            // Create the logic vertex for this statement
            UASSERT_OBJ(!currp->user3p(), currp, "user3p should not be set");
            Reorder_Logic_رأس* ثابت vtxp = جديد Reorder_Logic_رأس{m_graphp, currp};
            currp->user3p(vtxp);

            // Visit the statement - this can recursively reorder sub statements
            m_stmtStackps.افزودن_پشت(vtxp);
            iterate(currp);
            m_stmtStackps.برداشت_پشت();
        }

        إذا (m_noReorderWhy) {  // Jump or something nasty
            UINFO(9, "  NoReorderBlock because " << m_noReorderWhy);
            أرجع;
        }

        // Reorder statements in this block
        cleanup_بلوک_گراف(nodep);
        reorder_بلوک(nodep);

        // 'nodep' might no longer be the head of the list, rewind
        طالما (nodep->backp()->nextp() == nodep) nodep = nodep->backp();

        // Delete vertexes and edges only applying to this block
        لكل (Ast_گره* currp = nodep; currp; currp = currp->nextp()) {
            currp->user3u().مقصد<Reorder_Logic_رأس*>()->unlinkDelete(m_graphp);
            currp->user3p(اشاره_تهی);
        }
    }

    // VISITORS
    فراغ بازدید(AstAlways* nodep) override {
        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_درون_Dly);
        VL_RESTORER(m_noReorderWhy);

        V3_گراف گراف;
        m_graphp = &گراف;
        m_impureVtxp = اشاره_تهی;
        m_درون_Dly = خطأ;
        m_noReorderWhy = اشاره_تهی;

        ثابت VNUser1_درون_Use user1_درون_Use;
        ثابت VNUser2_درون_Use user2_درون_Use;
        ثابت VNUser3_درون_Use user3_درون_Use;
        ثابت VNUser4_درون_Use user4_درون_Use;

        UINFOTREE(9, nodep, "", "alwIn:");
        process_بلوک(nodep->stmtsp());
        UINFOTREE(9, nodep, "", "alwOut");

        m_stmtStackps.clear();
    }

    فراغ بازدید(Ast_گره_If* nodep) override {
        إذا (!m_graphp || m_noReorderWhy) أرجع;
        iterate_And_بعدی_Null(nodep->condp());
        process_بلوک(nodep->thensp());
        process_بلوک(nodep->elsesp());
    }

    فراغ بازدید(AstJumpGo* nodep) override {
        إذا (!m_graphp || m_noReorderWhy) أرجع;
        m_noReorderWhy = "JumpGo";
        iterateChildren(nodep);
    }

    فراغ بازدید(Ast_گره_CCall* nodep) override {
        إذا (!m_graphp || m_noReorderWhy) أرجع;
        // The callee is not inlined, so the variables it reads and writes carry no VarRef at
        // the call site, and the scoreboard would let assignments to them move across the
        // call. Treat any such call as a barrier.
        m_noReorderWhy = "CCall";
        iterateChildren(nodep);
    }

    فراغ بازدید(Ast_عبارت_Stmt* nodep) override {
        إذا (!m_graphp || m_noReorderWhy) أرجع;
        VL_RESTORER(m_درون_Dly);
        m_درون_Dly = خطأ;
        iterateChildren(nodep);
    }

    فراغ بازدید(Ast_تخصیص_بده_Dly* nodep) override {
        إذا (!m_graphp || m_noReorderWhy) أرجع;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        إذا (Ast_گره* ثابت controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_درون_Dly);
        m_درون_Dly = درست;
        iterate(nodep->lhsp());
    }

    فراغ بازدید(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        إذا (m_graphp) m_noReorderWhy = "Handle";
        بازدید(static_cast<Ast_گره*>(nodep));
    }

    فراغ بازدید(AstVarRef* nodep) override {
        إذا (!m_graphp || m_noReorderWhy) أرجع;
        إذا (m_stmtStackps.خالی()) أرجع;

        // Reads of constants can be ignored - TODO: This should be "constexpr", not run-time const
        إذا (nodep->varp()->isConst()) أرجع;

        // SPEEDUP: We add duplicate edges, that should be fixed

        AstVarScope* ثابت vscp = nodep->varScopep();

        // Create vertexes for variable
        إذا (!vscp->user1p()) vscp->user1p(جديد Reorder_Var_Std_رأس{m_graphp, vscp});
        Reorder_Var_Std_رأس* ثابت vstdp = vscp->user1u().مقصد<Reorder_Var_Std_رأس*>();

        // Variable is read
        إذا (nodep->access().isReadOnly()) {
            لكل (Reorder_Logic_رأس* ثابت vtxp : m_stmtStackps) {
                جديد ReorderRVEdge{m_graphp, vtxp, vstdp};
            }
            أرجع;
        }

        // Variable is written, not NBA
        إذا (!m_درون_Dly) {
            لكل (Reorder_Logic_رأس* ثابت vtxp : m_stmtStackps) {
                جديد ReorderLVEdge{m_graphp, vstdp, vtxp};
            }
            أرجع;
        }

        // Variable is written by NBA
        إذا (!vscp->user2p()) {
            Reorder_Var_Post_رأس* ثابت vpostp = جديد Reorder_Var_Post_رأس{m_graphp, vscp};
            vscp->user2p(vpostp);
            جديد Reorder_Post_یال{m_graphp, vstdp, vpostp};
        }
        Reorder_Var_Post_رأس* ثابت vpostp = vscp->user2u().مقصد<Reorder_Var_Post_رأس*>();
        لكل (Reorder_Logic_رأس* ثابت vtxp : m_stmtStackps) {
            جديد ReorderLVEdge{m_graphp, vpostp, vtxp};
        }
    }

    فراغ بازدید(Ast_گره* nodep) override {
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
            إذا (!m_impureVtxp) m_impureVtxp = جديد Reorder_Impure_رأس{m_graphp, nodep};
            // This edge is only used to find weakly connected components, so one edge is enough
            لكل (Reorder_Logic_رأس* ثابت vtxp : m_stmtStackps) {
                جديد Reorder_Scorebd_یال{m_graphp, m_impureVtxp, vtxp};
            }
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
عام:
    صریح ReorderVisitor(AstNetlist* nodep) { iterate(nodep); }
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
