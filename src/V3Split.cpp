// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Break always into separate statements
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
// V3Split transformation:
//
//  splitAll() splits large always blocks into smaller always blocks when
//  possible, without changing the order of dependent statements relative to
//  one another. Splitting is not limited to top-level statements, if-else
//  blocks can also be split, so that:
//
//    always @ (...) begin
//      if (reset) begin
//        a <= 0;
//        b <= 0;
//         // ... ten thousand more
//      end
//      else begin
//        a <= a_in;
//        b <= b_in;
//         // ... ten thousand more
//      end
//    end
//
// becomes a separate block for each of a, b, and so on.  Even though this
// requires duplicating the conditional many times, it's usually better as it
// reduces ordering constraints, and later optimizations can merge
// conditionals.
//
// To find what must stay together, a graph is built per always block, holding
// a vertex per 'leaf' and 'if' statement, and up to two vertices per variable.
// Statements in the same connected component must stay in one block, and each
// component then becomes a block of its own. The edges are:
//
//   - Blocking write: variable -> statement. Such a write is observable within
//     the block, so the readers of the variable stay with the writer.
//   - Non-blocking write: a separate 'post' vertex of the variable -> statement.
//     All writers of a variable stay together, but the readers, which see the
//     value from before the NBA commits, are not held together with them.
//   - Read: statement -> variable. For an 'if', only the reads in its own
//     condition count, not those in its branches.
//   - Impure statement: statement -> a vertex shared by all of them, so that
//     $display, DPI calls, etc stay in one block, in order.
//
// A variable with no blocking write is an input to the block, so its vertex is
// removed, and with it the dependencies on it, as two statements both reading
// an input need not stay together. An 'if' left with no dependencies of its
// own is removed likewise, so that the statements under it can separate, each
// taking a copy of the condition.
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Split.h"

#ضمن "V3Graph.h"
#ضمن "V3Stats.h"

#ضمن <string>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Support classes

صنف وېشل_غوټه_راس VL_NOT_FINAL : عام V3_ګراف_راس {
    VL_RTTI_IMPL(وېشل_غوټه_راس, V3_ګراف_راس)
    Ast_غوټه* ثابت m_nodep;

محمي:
    وېشل_غوټه_راس(V3_ګراف* graphp, Ast_غوټه* nodep)
        : V3_ګراف_راس{graphp}
        , m_nodep{nodep} {}
    // ACCESSORS
    std::string name() ثابت override {
        أرجع cvt_منزل_Hex(m_nodep) + ' ' + m_nodep->prettyTypeName();
    }

عام:
    Ast_غوټه* nodep() ثابت { أرجع m_nodep; }
};

صنف وېشل_Impure_راس final : عام وېشل_غوټه_راس {
    VL_RTTI_IMPL(وېشل_Impure_راس, وېشل_غوټه_راس)

    std::string name() ثابت override { أرجع "*IMPURE*"; }
    std::string dotColor() ثابت override { أرجع "green"; }

عام:
    څرګند وېشل_Impure_راس(V3_ګراف* graphp, Ast_غوټه* nodep)
        : وېشل_غوټه_راس{graphp, nodep} {}
};

صنف وېشل_Stmt_راس final : عام وېشل_غوټه_راس {
    VL_RTTI_IMPL(وېشل_Stmt_راس, وېشل_غوټه_راس)

    std::string dotColor() ثابت override { أرجع "yellow"; }

عام:
    وېشل_Stmt_راس(V3_ګراف* graphp, Ast_غوټه* nodep)
        : وېشل_غوټه_راس{graphp, nodep} {}
};

صنف وېشل_Var_Std_راس final : عام وېشل_غوټه_راس {
    VL_RTTI_IMPL(وېشل_Var_Std_راس, وېشل_غوټه_راس)

    std::string dotColor() ثابت override { أرجع "skyblue"; }

عام:
    وېشل_Var_Std_راس(V3_ګراف* graphp, AstVarScope* vscp)
        : وېشل_غوټه_راس{graphp, vscp} {}
};

صنف وېشل_Var_Post_راس final : عام وېشل_غوټه_راس {
    VL_RTTI_IMPL(وېشل_Var_Post_راس, وېشل_غوټه_راس)

    std::string name() ثابت override { أرجع "POST "s + وېشل_غوټه_راس::name(); }
    std::string dotColor() ثابت override { أرجع "CadetBlue"; }

عام:
    وېشل_Var_Post_راس(V3_ګراف* graphp, AstVarScope* vscp)
        : وېشل_غوټه_راس{graphp, vscp} {}
};

صنف وېشل_Visitor final : عام VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> SplitVarStdVertex*: Regular program-flow variable vertex
    // AstVarScope::user2p  -> SplitVarPostVertex*: NBA written delayed variable vertex
    // Ast{StmtIsh}::user1p -> SplitStmtVertex*

    // NODE STATE
    // AstAlways::user3     -> bool: Block created by splitting, needs no further splitting
    ثابت VNUser3_دننه_Use m_inuser3;

    // STATE
    V3_ګراف* m_graphp = تش_اشاره;  // Dependency graph to analyze statement connectivity
    std::ویکټور<وېشل_Stmt_راس*> m_stmtStackps;  // Current statements being tracked
    وېشل_Impure_راس* m_impureVtxp = تش_اشاره;  // Vertex connecting impure statements
    ثابت حرف* m_no_وېشل_Why = تش_اشاره;  // Reason current block cannot be split
    منطقي m_دننه_Dly = خطأ;  // Inside AstAssignDly Lhs
    ثابت AstIf* m_currIfp = تش_اشاره;  // The AstIf whose condition is currently visited
    VDouble0 m_statSplits;  // Statistic tracking

    // METHODS
    فراغ جمع_کړه_څنډه(V3_ګراف_راس* fromp, V3_ګراف_راس* سر) {
        جديد V3_ګراف_څنډه{m_graphp, fromp, سر, 1};
    }

    // Iterate the given list of statements, building the dependency graph
    فراغ لټون_بلاک(Ast_غوټه* stmtsp) {
        إذا (m_no_وېشل_Why) أرجع;
        لكل (Ast_غوټه* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Skip comments. They have no dependencies at all, so would always
            // form an independent component, and hence a split block, of their
            // own, which would be subsequently deleted as it does nothing.
            إذا (VN_IS(stmtp, Comment)) استمر;
            UASSERT_OBJ(!stmtp->user1p(), stmtp, "user1p should not be set");
            وېشل_Stmt_راس* ثابت vtxp = جديد وېشل_Stmt_راس{m_graphp, stmtp};
            stmtp->user1p(vtxp);
            m_stmtStackps.ټېل_شا(vtxp);
            iterate(stmtp);
            m_stmtStackps.راوباسه_شا();
        }
    }

    // Remove unnecessary edges, then color to find weakly connected components
    uint32_t color_Always_ګراف() {
        إذا (dump_ګراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_built", خطأ);

        // Prune duplicate edges. Not necessary for correctness, but simplifies dumps.
        m_graphp->remove_Redundant_څنډې_Max(&V3_ګراف_څنډه::followAlwaysTrue);
        إذا (dump_ګراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nodup", خطأ);

        // Remove variable vertices that are not written in the block.
        // These are input-only to the block, so carry no dependency.
        لكل (V3_ګراف_راس* ثابت vtxp : m_graphp->vertices().unlinkable()) {
            وېشل_Var_Std_راس* ثابت vstdp = vtxp->cast<وېشل_Var_Std_راس>();
            إذا (!vstdp || !vstdp->بهر_تشه()) استمر;
            UINFOTREE(9, vstdp->nodep(), "", "Will remove deps on block input var:");
            vstdp->nodep()->user1p(تش_اشاره);  // Don't leave a dangling pointer behind
            VL_DO_DANGLING(vstdp->unlinkDelete(m_graphp), vstdp);
        }
        إذا (dump_ګراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_noinputs", خطأ);

        // A statement under an 'if' also has an edge to the 'if' itself, from
        // each variable it writes, so an 'if' holds its whole body together.
        // An 'if' has out edges only for what its own condition reads. If
        // after the pruning of block inputs above, an 'if' has no remaining
        // out edges (dependencies of its condition), then it constrains
        // nothing. If so, then remove the 'if' statement vertex, so
        // its contents can split apart, each part taking a copy of the
        // condition.  This is what allows splitting within an if/else at all,
        // and is what breaks up the reset tree in the example at the top of
        // this file.
        لكل (V3_ګراف_راس* ثابت vtxp : m_graphp->vertices().unlinkable()) {
            وېشل_Stmt_راس* ثابت stmtVtxp = vtxp->cast<وېشل_Stmt_راس>();
            إذا (!stmtVtxp || !VN_IS(stmtVtxp->nodep(), If)) استمر;
            // Can't remove if dependent on a variable written in the block
            إذا (!stmtVtxp->بهر_تشه()) استمر;
            // Depends only on block inputs, so can be split. Remove the vertex.
            stmtVtxp->nodep()->user1p(تش_اشاره);
            stmtVtxp->unlinkDelete(m_graphp);
        }
        إذا (dump_ګراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nofreeifs", خطأ);

        // Weak coloring to determine what must stay together in a single block
        ثابت uint32_t عدد_Colors = m_graphp->weaklyConnected(&V3_ګراف_څنډه::followAlwaysTrue);
        إذا (dump_ګراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_colored", خطأ);
        أرجع عدد_Colors;
    }

    // Take the statements of the given list, and return them distributed into one list per color.
    ساكن std::ویکټور<Ast_غوټه*> وېشل_Statements(Ast_غوټه* stmtsp, uint32_t عدد_Colors) {
        std::ویکټور<Ast_غوټه*> پایله{عدد_Colors, تش_اشاره};
        لكل (Ast_غوټه *stmtp = stmtsp, *nextp = تش_اشاره; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();  // 'stmtp' is unlinked below

            // Comments are dropped if the block is split
            إذا (VN_IS(stmtp, Comment)) استمر;

            // Pick up the statement vertex, which might be nullptr after pruning during analysis
            ثابت وېشل_Stmt_راس* ثابت vtxp = stmtp->user1u().منزل<وېشل_Stmt_راس*>();

            // If statements are duplicated for each split branch
            إذا (AstIf* ثابت ifp = VN_CAST(stmtp, If)) {
                ثابت خپلکاری thens = وېشل_Statements(ifp->thensp(), عدد_Colors);
                ثابت خپلکاری elses = وېشل_Statements(ifp->elsesp(), عدد_Colors);
                File_کرښه* ثابت flp = ifp->fileline();
                // Rebuild the 'if' for each color present in either branch
                منطقي تشه = ریښتیا;
                لكل (uint32_t color = 0; color < عدد_Colors; ++color) {
                    إذا (!thens[color] && !elses[color]) استمر;
                    تشه = خطأ;
                    // The condition is cloned for each color. An impure condition
                    // keeps the 'if' and all it holds in one component, so just once.
                    AstIf* ثابت clonep = جديد AstIf{flp, ifp->condp()->clone_ونه(ریښتیا),
                                                    thens[color], elses[color]};
                    // Preserve pragmas from unique if's so assertions work properly
                    clonep->uniquePragma(ifp->uniquePragma());
                    clonep->unique0Pragma(ifp->unique0Pragma());
                    clonep->لومړیتوب_Pragma(ifp->لومړیتوب_Pragma());
                    پایله[color] = Ast_غوټه::جمع_کړه_بل(پایله[color], clonep);
                }
                // There is nothing under the 'if' to guard. If its vertex was
                // removed as having no dependencies at all, then its
                // condition reads only block inputs and is pure, so the whole
                // 'if' can go. Otherwise the condition might have a side
                // effect, so keep just the condition, evaluated as a
                // statement, under the color of the 'if' itself.
                إذا (تشه && vtxp) {
                    ثابت uint32_t color = vtxp->color();
                    Ast_غوټه_بیان* ثابت condp = ifp->condp();
                    condp->unlink_Fr_شا();
                    پایله[color] = Ast_غوټه::جمع_کړه_بل(پایله[color], جديد Ast_Stmt_بیان{flp, condp});
                }
                استمر;
            }

            // Move the leaf into its color's list
            ثابت uint32_t color = vtxp->color();
            پایله[color] = Ast_غوټه::جمع_کړه_بل(پایله[color], stmtp->unlink_Fr_شا());
        }
        أرجع پایله;
    }

    // VISITORS
    فراغ لیدنه(AstAlways* nodep) override {
        // Skip blocks created below
        إذا (nodep->user3()) أرجع;

        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_no_وېشل_Why);
        VL_RESTORER(m_دننه_Dly);
        V3_ګراف ګراف;
        m_graphp = &ګراف;
        m_impureVtxp = تش_اشاره;
        m_no_وېشل_Why = تش_اشاره;
        m_دننه_Dly = خطأ;
        UASSERT_OBJ(m_stmtStackps.تشه(), nodep, "Statement stack not empty");

        // Build the graph
        ثابت VNUser1_دننه_Use user1_دننه_Use;
        ثابت VNUser2_دننه_Use user2_دننه_Use;
        لټون_بلاک(nodep->stmtsp());

        // We might have to give up
        إذا (m_no_وېشل_Why) {
            UINFO(9, "  NoSplitBlock because " << m_no_وېشل_Why);
            أرجع;
        }

        // Color the graph to identify separable statements
        ثابت uint32_t عدد_Colors = color_Always_ګراف();
        // If the whole block is one component (or empty), then nothing to split
        إذا (عدد_Colors <= 1) أرجع;

        UINFO(6, "  splitting always " << nodep);

        // Count the number of new blocks inserted into the Ast: '1 -> n' split, so 'n - 1' extra
        m_statSplits += عدد_Colors - 1;

        // Unpick the statements out of the original block, into one list per color
        ثابت خپلکاری lists = وېشل_Statements(nodep->stmtsp(), عدد_Colors);
        UASSERT_OBJ(lists.اندازه() == عدد_Colors, nodep, "Inconsistent split");

        // Whatever 'splitStatements' did not take, (comments, empty ifs) is not needed any more
        إذا (Ast_غوټه* ثابت restp = nodep->stmtsp()) {
            VL_DO_DANGLING(restp->unlink_Fr_شا_With_بل()->delete_ونه(), restp);
        }

        // Every color has a statement in it. Reuse the original block for the
        // first color, and add a new block after it for each of the rest.
        UASSERT_OBJ(lists.front(), nodep, "Color with no statements");
        nodep->جمع_کړه_Stmtsp(lists.front());
        Ast_غوټه* lastp = nodep;
        File_کرښه* ثابت flp = nodep->fileline();
        ثابت VAlwaysKwd kwd = nodep->keyword();
        لكل (اندازه_t i = 1; i < عدد_Colors; ++i) {
            Ast_غوټه* ثابت stmtsp = lists[i];
            UASSERT_OBJ(stmtsp, nodep, "Color with no statements");
            AstAlways* ثابت newp = جديد AstAlways{flp, kwd, تش_اشاره, stmtsp};
            newp->user3(1);  // Do not split again
            lastp->جمع_کړه_بل_دلته(newp);
            lastp = newp;
        }
    }

    فراغ لیدنه(AstIf* nodep) override {
        إذا (!m_graphp || m_no_وېشل_Why) أرجع;
        {
            VL_RESTORER(m_currIfp);
            m_currIfp = nodep;
            iterate_And_بل_Null(nodep->condp());
        }
        لټون_بلاک(nodep->thensp());
        لټون_بلاک(nodep->elsesp());
    }

    فراغ لیدنه(Ast_بیان_Stmt* nodep) override {
        إذا (!m_graphp || m_no_وېشل_Why) أرجع;
        VL_RESTORER(m_دننه_Dly);
        m_دننه_Dly = خطأ;
        iterateChildren(nodep);
    }

    فراغ لیدنه(Ast_ټاکل_Dly* nodep) override {
        إذا (!m_graphp || m_no_وېشل_Why) أرجع;
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
        إذا (m_graphp) m_no_وېشل_Why = "Handle";
        لیدنه(static_cast<Ast_غوټه*>(nodep));
    }

    فراغ لیدنه(AstJumpGo*) override {
        إذا (!m_graphp || m_no_وېشل_Why) أرجع;
        m_no_وېشل_Why = "JumpGo";
    }

    فراغ لیدنه(AstVarRef* nodep) override {
        إذا (!m_graphp || m_no_وېشل_Why) أرجع;
        UASSERT_OBJ(!m_stmtStackps.تشه(), nodep, "Not under a statement");

        // Constant lookups can be ignored
        إذا (nodep->varp()->isConst()) أرجع;

        AstVarScope* ثابت vscp = nodep->varScopep();

        // SPEEDUP: We add duplicate edges, that should be fixed
        إذا (m_دننه_Dly && nodep->access().isWriteOrRW()) {
            // Delayed variable: is different from non-delayed variable, writes to it
            // are not observable while executing this block (NBA not yet committed),
            // so add only a write edge to a separate 'post' vertex.
            إذا (!vscp->user2p()) vscp->user2p(جديد وېشل_Var_Post_راس{m_graphp, vscp});
            وېشل_Var_Post_راس* ثابت vpostp = vscp->user2u().منزل<وېشل_Var_Post_راس*>();
            لكل (وېشل_Stmt_راس* ثابت vtxp : m_stmtStackps) جمع_کړه_څنډه(vpostp, vtxp);
        } وإلا إذا (nodep->access().isWriteOrRW()) {
            // Regular (non-blocking) write: Need to maintain program-flow order
            إذا (!vscp->user1p()) vscp->user1p(جديد وېشل_Var_Std_راس{m_graphp, vscp});
            وېشل_Var_Std_راس* ثابت vstdp = vscp->user1u().منزل<وېشل_Var_Std_راس*>();
            لكل (وېشل_Stmt_راس* ثابت vtxp : m_stmtStackps) جمع_کړه_څنډه(vstdp, vtxp);
        } وإلا {
            // Regular (non-blocking) read: Need to maintain program-flow order
            إذا (!vscp->user1p()) vscp->user1p(جديد وېشل_Var_Std_راس{m_graphp, vscp});
            وېشل_Var_Std_راس* ثابت vstdp = vscp->user1u().منزل<وېشل_Var_Std_راس*>();
            لكل (وېشل_Stmt_راس* ثابت vtxp : m_stmtStackps) {
                // If this is an if statement it only depends on refs in its
                // own condition only (not those in its branches). For other
                // statements, just record the referene as normal.
                إذا (ثابت AstIf* ثابت ifp = VN_CAST(vtxp->nodep(), If)) {
                    إذا (ifp != m_currIfp) استمر;
                }
                جمع_کړه_څنډه(vtxp, vstdp);
            }
        }
    }

    فراغ لیدنه(Ast_غوټه* nodep) override {
        // Outside AstAlways, just descend
        إذا (!m_graphp) {
            iterateChildren(nodep);
            أرجع;
        }
        // Early exit if decided not to split
        إذا (m_no_وېشل_Why) أرجع;

        UASSERT_OBJ(!m_stmtStackps.تشه(), nodep, "Not under a statement");

        // Timing control prevents splitting
        إذا (nodep->isTimingControl()) {
            m_no_وېشل_Why = "TimingControl";
            أرجع;
        }

        // All impure statements must be grouped together.
        إذا (!nodep->is_خالص()) {
            إذا (!m_impureVtxp) m_impureVtxp = جديد وېشل_Impure_راس{m_graphp, nodep};
            // One edge is enough to find the weakly connected components, but
            // it must point at the impure vertex, so it is an out edge (input
            // dependency) of any enclosing 'if' to prevent pruning.
            لكل (وېشل_Stmt_راس* ثابت vtxp : m_stmtStackps) جمع_کړه_څنډه(vtxp, m_impureVtxp);
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
    څرګند وېشل_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~وېشل_Visitor() override { V3Stats::جمع_کړه_Stat("Optimizations, Split always", m_statSplits); }
    VL_UNCOPYABLE(وېشل_Visitor);

عام:
    ساكن فراغ apply(AstNetlist* nodep) { وېشل_Visitor{nodep}; }
};

//######################################################################
// Split class functions

فراغ V3_وېشل::وېشل_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    وېشل_Visitor::apply(nodep);
    V3Global::dump_Check_Global_ونه("split", 0, dump_ونه_Either_Level() >= 3);
}
