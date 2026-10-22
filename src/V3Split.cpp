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

صنف ورهايو_ڳنڍ_راس VL_NOT_FINAL : عام V3_گراف_راس {
    VL_RTTI_IMPL(ورهايو_ڳنڍ_راس, V3_گراف_راس)
    Ast_ڳنڍ* ثابت m_nodep;

محمي:
    ورهايو_ڳنڍ_راس(V3_گراف* graphp, Ast_ڳنڍ* nodep)
        : V3_گراف_راس{graphp}
        , m_nodep{nodep} {}
    // ACCESSORS
    std::string name() ثابت override {
        أرجع cvt_منزل_Hex(m_nodep) + ' ' + m_nodep->prettyTypeName();
    }

عام:
    Ast_ڳنڍ* nodep() ثابت { أرجع m_nodep; }
};

صنف ورهايو_Impure_راس final : عام ورهايو_ڳنڍ_راس {
    VL_RTTI_IMPL(ورهايو_Impure_راس, ورهايو_ڳنڍ_راس)

    std::string name() ثابت override { أرجع "*IMPURE*"; }
    std::string dotColor() ثابت override { أرجع "green"; }

عام:
    واضح ورهايو_Impure_راس(V3_گراف* graphp, Ast_ڳنڍ* nodep)
        : ورهايو_ڳنڍ_راس{graphp, nodep} {}
};

صنف ورهايو_Stmt_راس final : عام ورهايو_ڳنڍ_راس {
    VL_RTTI_IMPL(ورهايو_Stmt_راس, ورهايو_ڳنڍ_راس)

    std::string dotColor() ثابت override { أرجع "yellow"; }

عام:
    ورهايو_Stmt_راس(V3_گراف* graphp, Ast_ڳنڍ* nodep)
        : ورهايو_ڳنڍ_راس{graphp, nodep} {}
};

صنف ورهايو_Var_Std_راس final : عام ورهايو_ڳنڍ_راس {
    VL_RTTI_IMPL(ورهايو_Var_Std_راس, ورهايو_ڳنڍ_راس)

    std::string dotColor() ثابت override { أرجع "skyblue"; }

عام:
    ورهايو_Var_Std_راس(V3_گراف* graphp, AstVarScope* vscp)
        : ورهايو_ڳنڍ_راس{graphp, vscp} {}
};

صنف ورهايو_Var_Post_راس final : عام ورهايو_ڳنڍ_راس {
    VL_RTTI_IMPL(ورهايو_Var_Post_راس, ورهايو_ڳنڍ_راس)

    std::string name() ثابت override { أرجع "POST "s + ورهايو_ڳنڍ_راس::name(); }
    std::string dotColor() ثابت override { أرجع "CadetBlue"; }

عام:
    ورهايو_Var_Post_راس(V3_گراف* graphp, AstVarScope* vscp)
        : ورهايو_ڳنڍ_راس{graphp, vscp} {}
};

صنف ورهايو_Visitor final : عام VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> SplitVarStdVertex*: Regular program-flow variable vertex
    // AstVarScope::user2p  -> SplitVarPostVertex*: NBA written delayed variable vertex
    // Ast{StmtIsh}::user1p -> SplitStmtVertex*

    // NODE STATE
    // AstAlways::user3     -> bool: Block created by splitting, needs no further splitting
    ثابت VNUser3_اندريون_Use m_inuser3;

    // STATE
    V3_گراف* m_graphp = خالي_اشارو;  // Dependency graph to analyze statement connectivity
    std::ويڪٽر<ورهايو_Stmt_راس*> m_stmtStackps;  // Current statements being tracked
    ورهايو_Impure_راس* m_impureVtxp = خالي_اشارو;  // Vertex connecting impure statements
    ثابت حرف* m_no_ورهايو_Why = خالي_اشارو;  // Reason current block cannot be split
    منطقي m_اندريون_Dly = خطأ;  // Inside AstAssignDly Lhs
    ثابت AstIf* m_currIfp = خالي_اشارو;  // The AstIf whose condition is currently visited
    VDouble0 m_statSplits;  // Statistic tracking

    // METHODS
    فراغ جوڙيو_ڪنارو(V3_گراف_راس* fromp, V3_گراف_راس* چوٽي) {
        جديد V3_گراف_ڪنارو{m_graphp, fromp, چوٽي, 1};
    }

    // Iterate the given list of statements, building the dependency graph
    فراغ جاچ_بلاڪ(Ast_ڳنڍ* stmtsp) {
        إذا (m_no_ورهايو_Why) أرجع;
        لكل (Ast_ڳنڍ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Skip comments. They have no dependencies at all, so would always
            // form an independent component, and hence a split block, of their
            // own, which would be subsequently deleted as it does nothing.
            إذا (VN_IS(stmtp, Comment)) استمر;
            UASSERT_OBJ(!stmtp->user1p(), stmtp, "user1p should not be set");
            ورهايو_Stmt_راس* ثابت vtxp = جديد ورهايو_Stmt_راس{m_graphp, stmtp};
            stmtp->user1p(vtxp);
            m_stmtStackps.ڌڪو_پٺ(vtxp);
            iterate(stmtp);
            m_stmtStackps.ڪڍو_پٺ();
        }
    }

    // Remove unnecessary edges, then color to find weakly connected components
    uint32_t color_Always_گراف() {
        إذا (dump_گراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_built", خطأ);

        // Prune duplicate edges. Not necessary for correctness, but simplifies dumps.
        m_graphp->remove_Redundant_ڪنارا_Max(&V3_گراف_ڪنارو::followAlwaysTrue);
        إذا (dump_گراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nodup", خطأ);

        // Remove variable vertices that are not written in the block.
        // These are input-only to the block, so carry no dependency.
        لكل (V3_گراف_راس* ثابت vtxp : m_graphp->vertices().unlinkable()) {
            ورهايو_Var_Std_راس* ثابت vstdp = vtxp->cast<ورهايو_Var_Std_راس>();
            إذا (!vstdp || !vstdp->ٻاهريون_خالي_جايون()) استمر;
            UINFOTREE(9, vstdp->nodep(), "", "Will remove deps on block input var:");
            vstdp->nodep()->user1p(خالي_اشارو);  // Don't leave a dangling pointer behind
            VL_DO_DANGLING(vstdp->unlinkDelete(m_graphp), vstdp);
        }
        إذا (dump_گراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_noinputs", خطأ);

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
        لكل (V3_گراف_راس* ثابت vtxp : m_graphp->vertices().unlinkable()) {
            ورهايو_Stmt_راس* ثابت stmtVtxp = vtxp->cast<ورهايو_Stmt_راس>();
            إذا (!stmtVtxp || !VN_IS(stmtVtxp->nodep(), If)) استمر;
            // Can't remove if dependent on a variable written in the block
            إذا (!stmtVtxp->ٻاهريون_خالي_جايون()) استمر;
            // Depends only on block inputs, so can be split. Remove the vertex.
            stmtVtxp->nodep()->user1p(خالي_اشارو);
            stmtVtxp->unlinkDelete(m_graphp);
        }
        إذا (dump_گراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nofreeifs", خطأ);

        // Weak coloring to determine what must stay together in a single block
        ثابت uint32_t انگ_Colors = m_graphp->weaklyConnected(&V3_گراف_ڪنارو::followAlwaysTrue);
        إذا (dump_گراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_colored", خطأ);
        أرجع انگ_Colors;
    }

    // Take the statements of the given list, and return them distributed into one list per color.
    ساكن std::ويڪٽر<Ast_ڳنڍ*> ورهايو_Statements(Ast_ڳنڍ* stmtsp, uint32_t انگ_Colors) {
        std::ويڪٽر<Ast_ڳنڍ*> نتيجو{انگ_Colors, خالي_اشارو};
        لكل (Ast_ڳنڍ *stmtp = stmtsp, *nextp = خالي_اشارو; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();  // 'stmtp' is unlinked below

            // Comments are dropped if the block is split
            إذا (VN_IS(stmtp, Comment)) استمر;

            // Pick up the statement vertex, which might be nullptr after pruning during analysis
            ثابت ورهايو_Stmt_راس* ثابت vtxp = stmtp->user1u().منزل<ورهايو_Stmt_راس*>();

            // If statements are duplicated for each split branch
            إذا (AstIf* ثابت ifp = VN_CAST(stmtp, If)) {
                ثابت خودڪار thens = ورهايو_Statements(ifp->thensp(), انگ_Colors);
                ثابت خودڪار elses = ورهايو_Statements(ifp->elsesp(), انگ_Colors);
                File_سٽ* ثابت flp = ifp->fileline();
                // Rebuild the 'if' for each color present in either branch
                منطقي خالي_جايون = سچ;
                لكل (uint32_t color = 0; color < انگ_Colors; ++color) {
                    إذا (!thens[color] && !elses[color]) استمر;
                    خالي_جايون = خطأ;
                    // The condition is cloned for each color. An impure condition
                    // keeps the 'if' and all it holds in one component, so just once.
                    AstIf* ثابت clonep = جديد AstIf{flp, ifp->condp()->clone_وڻ(سچ),
                                                    thens[color], elses[color]};
                    // Preserve pragmas from unique if's so assertions work properly
                    clonep->uniquePragma(ifp->uniquePragma());
                    clonep->unique0Pragma(ifp->unique0Pragma());
                    clonep->ترجيح_Pragma(ifp->ترجيح_Pragma());
                    نتيجو[color] = Ast_ڳنڍ::جوڙيو_ايندڙ(نتيجو[color], clonep);
                }
                // There is nothing under the 'if' to guard. If its vertex was
                // removed as having no dependencies at all, then its
                // condition reads only block inputs and is pure, so the whole
                // 'if' can go. Otherwise the condition might have a side
                // effect, so keep just the condition, evaluated as a
                // statement, under the color of the 'if' itself.
                إذا (خالي_جايون && vtxp) {
                    ثابت uint32_t color = vtxp->color();
                    Ast_ڳنڍ_اظهار* ثابت condp = ifp->condp();
                    condp->unlink_Fr_پٺ();
                    نتيجو[color] = Ast_ڳنڍ::جوڙيو_ايندڙ(نتيجو[color], جديد Ast_Stmt_اظهار{flp, condp});
                }
                استمر;
            }

            // Move the leaf into its color's list
            ثابت uint32_t color = vtxp->color();
            نتيجو[color] = Ast_ڳنڍ::جوڙيو_ايندڙ(نتيجو[color], stmtp->unlink_Fr_پٺ());
        }
        أرجع نتيجو;
    }

    // VISITORS
    فراغ گهمو(AstAlways* nodep) override {
        // Skip blocks created below
        إذا (nodep->user3()) أرجع;

        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_no_ورهايو_Why);
        VL_RESTORER(m_اندريون_Dly);
        V3_گراف گراف;
        m_graphp = &گراف;
        m_impureVtxp = خالي_اشارو;
        m_no_ورهايو_Why = خالي_اشارو;
        m_اندريون_Dly = خطأ;
        UASSERT_OBJ(m_stmtStackps.خالي_جايون(), nodep, "Statement stack not empty");

        // Build the graph
        ثابت VNUser1_اندريون_Use user1_اندريون_Use;
        ثابت VNUser2_اندريون_Use user2_اندريون_Use;
        جاچ_بلاڪ(nodep->stmtsp());

        // We might have to give up
        إذا (m_no_ورهايو_Why) {
            UINFO(9, "  NoSplitBlock because " << m_no_ورهايو_Why);
            أرجع;
        }

        // Color the graph to identify separable statements
        ثابت uint32_t انگ_Colors = color_Always_گراف();
        // If the whole block is one component (or empty), then nothing to split
        إذا (انگ_Colors <= 1) أرجع;

        UINFO(6, "  splitting always " << nodep);

        // Count the number of new blocks inserted into the Ast: '1 -> n' split, so 'n - 1' extra
        m_statSplits += انگ_Colors - 1;

        // Unpick the statements out of the original block, into one list per color
        ثابت خودڪار lists = ورهايو_Statements(nodep->stmtsp(), انگ_Colors);
        UASSERT_OBJ(lists.سائيز() == انگ_Colors, nodep, "Inconsistent split");

        // Whatever 'splitStatements' did not take, (comments, empty ifs) is not needed any more
        إذا (Ast_ڳنڍ* ثابت restp = nodep->stmtsp()) {
            VL_DO_DANGLING(restp->unlink_Fr_پٺ_With_ايندڙ()->delete_وڻ(), restp);
        }

        // Every color has a statement in it. Reuse the original block for the
        // first color, and add a new block after it for each of the rest.
        UASSERT_OBJ(lists.front(), nodep, "Color with no statements");
        nodep->جوڙيو_Stmtsp(lists.front());
        Ast_ڳنڍ* lastp = nodep;
        File_سٽ* ثابت flp = nodep->fileline();
        ثابت VAlwaysKwd kwd = nodep->keyword();
        لكل (سائيز_t i = 1; i < انگ_Colors; ++i) {
            Ast_ڳنڍ* ثابت stmtsp = lists[i];
            UASSERT_OBJ(stmtsp, nodep, "Color with no statements");
            AstAlways* ثابت newp = جديد AstAlways{flp, kwd, خالي_اشارو, stmtsp};
            newp->user3(1);  // Do not split again
            lastp->جوڙيو_ايندڙ_هتي(newp);
            lastp = newp;
        }
    }

    فراغ گهمو(AstIf* nodep) override {
        إذا (!m_graphp || m_no_ورهايو_Why) أرجع;
        {
            VL_RESTORER(m_currIfp);
            m_currIfp = nodep;
            iterate_And_ايندڙ_Null(nodep->condp());
        }
        جاچ_بلاڪ(nodep->thensp());
        جاچ_بلاڪ(nodep->elsesp());
    }

    فراغ گهمو(Ast_اظهار_Stmt* nodep) override {
        إذا (!m_graphp || m_no_ورهايو_Why) أرجع;
        VL_RESTORER(m_اندريون_Dly);
        m_اندريون_Dly = خطأ;
        iterateChildren(nodep);
    }

    فراغ گهمو(Ast_سونپيو_Dly* nodep) override {
        إذا (!m_graphp || m_no_ورهايو_Why) أرجع;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        إذا (Ast_ڳنڍ* ثابت controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_اندريون_Dly);
        m_اندريون_Dly = سچ;
        iterate(nodep->lhsp());
    }

    فراغ گهمو(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        إذا (m_graphp) m_no_ورهايو_Why = "Handle";
        گهمو(static_cast<Ast_ڳنڍ*>(nodep));
    }

    فراغ گهمو(AstJumpGo*) override {
        إذا (!m_graphp || m_no_ورهايو_Why) أرجع;
        m_no_ورهايو_Why = "JumpGo";
    }

    فراغ گهمو(AstVarRef* nodep) override {
        إذا (!m_graphp || m_no_ورهايو_Why) أرجع;
        UASSERT_OBJ(!m_stmtStackps.خالي_جايون(), nodep, "Not under a statement");

        // Constant lookups can be ignored
        إذا (nodep->varp()->isConst()) أرجع;

        AstVarScope* ثابت vscp = nodep->varScopep();

        // SPEEDUP: We add duplicate edges, that should be fixed
        إذا (m_اندريون_Dly && nodep->access().isWriteOrRW()) {
            // Delayed variable: is different from non-delayed variable, writes to it
            // are not observable while executing this block (NBA not yet committed),
            // so add only a write edge to a separate 'post' vertex.
            إذا (!vscp->user2p()) vscp->user2p(جديد ورهايو_Var_Post_راس{m_graphp, vscp});
            ورهايو_Var_Post_راس* ثابت vpostp = vscp->user2u().منزل<ورهايو_Var_Post_راس*>();
            لكل (ورهايو_Stmt_راس* ثابت vtxp : m_stmtStackps) جوڙيو_ڪنارو(vpostp, vtxp);
        } وإلا إذا (nodep->access().isWriteOrRW()) {
            // Regular (non-blocking) write: Need to maintain program-flow order
            إذا (!vscp->user1p()) vscp->user1p(جديد ورهايو_Var_Std_راس{m_graphp, vscp});
            ورهايو_Var_Std_راس* ثابت vstdp = vscp->user1u().منزل<ورهايو_Var_Std_راس*>();
            لكل (ورهايو_Stmt_راس* ثابت vtxp : m_stmtStackps) جوڙيو_ڪنارو(vstdp, vtxp);
        } وإلا {
            // Regular (non-blocking) read: Need to maintain program-flow order
            إذا (!vscp->user1p()) vscp->user1p(جديد ورهايو_Var_Std_راس{m_graphp, vscp});
            ورهايو_Var_Std_راس* ثابت vstdp = vscp->user1u().منزل<ورهايو_Var_Std_راس*>();
            لكل (ورهايو_Stmt_راس* ثابت vtxp : m_stmtStackps) {
                // If this is an if statement it only depends on refs in its
                // own condition only (not those in its branches). For other
                // statements, just record the referene as normal.
                إذا (ثابت AstIf* ثابت ifp = VN_CAST(vtxp->nodep(), If)) {
                    إذا (ifp != m_currIfp) استمر;
                }
                جوڙيو_ڪنارو(vtxp, vstdp);
            }
        }
    }

    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        // Outside AstAlways, just descend
        إذا (!m_graphp) {
            iterateChildren(nodep);
            أرجع;
        }
        // Early exit if decided not to split
        إذا (m_no_ورهايو_Why) أرجع;

        UASSERT_OBJ(!m_stmtStackps.خالي_جايون(), nodep, "Not under a statement");

        // Timing control prevents splitting
        إذا (nodep->isTimingControl()) {
            m_no_ورهايو_Why = "TimingControl";
            أرجع;
        }

        // All impure statements must be grouped together.
        إذا (!nodep->is_خالص()) {
            إذا (!m_impureVtxp) m_impureVtxp = جديد ورهايو_Impure_راس{m_graphp, nodep};
            // One edge is enough to find the weakly connected components, but
            // it must point at the impure vertex, so it is an out edge (input
            // dependency) of any enclosing 'if' to prevent pruning.
            لكل (ورهايو_Stmt_راس* ثابت vtxp : m_stmtStackps) جوڙيو_ڪنارو(vtxp, m_impureVtxp);
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
    واضح ورهايو_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~ورهايو_Visitor() override { V3Stats::جوڙيو_Stat("Optimizations, Split always", m_statSplits); }
    VL_UNCOPYABLE(ورهايو_Visitor);

عام:
    ساكن فراغ apply(AstNetlist* nodep) { ورهايو_Visitor{nodep}; }
};

//######################################################################
// Split class functions

فراغ V3_ورهايو::ورهايو_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    ورهايو_Visitor::apply(nodep);
    V3Global::dump_Check_Global_وڻ("split", 0, dump_وڻ_Either_Level() >= 3);
}
