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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Split.h"

#شامل "V3Graph.h"
#شامل "V3Stats.h"

#شامل <string>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Support classes

جماعت بانٹو_گرہ_راس VL_NOT_FINAL : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(بانٹو_گرہ_راس, V3_گراف_راس)
    Ast_گرہ* ثابت m_nodep;

محفوظ:
    بانٹو_گرہ_راس(V3_گراف* graphp, Ast_گرہ* nodep)
        : V3_گراف_راس{graphp}
        , m_nodep{nodep} {}
    // ACCESSORS
    std::string name() ثابت override {
        واپس cvt_مقصد_Hex(m_nodep) + ' ' + m_nodep->prettyTypeName();
    }

عوامی:
    Ast_گرہ* nodep() ثابت { واپس m_nodep; }
};

جماعت بانٹو_Impure_راس final : عوامی بانٹو_گرہ_راس {
    VL_RTTI_IMPL(بانٹو_Impure_راس, بانٹو_گرہ_راس)

    std::string name() ثابت override { واپس "*IMPURE*"; }
    std::string dotColor() ثابت override { واپس "green"; }

عوامی:
    صریح بانٹو_Impure_راس(V3_گراف* graphp, Ast_گرہ* nodep)
        : بانٹو_گرہ_راس{graphp, nodep} {}
};

جماعت بانٹو_Stmt_راس final : عوامی بانٹو_گرہ_راس {
    VL_RTTI_IMPL(بانٹو_Stmt_راس, بانٹو_گرہ_راس)

    std::string dotColor() ثابت override { واپس "yellow"; }

عوامی:
    بانٹو_Stmt_راس(V3_گراف* graphp, Ast_گرہ* nodep)
        : بانٹو_گرہ_راس{graphp, nodep} {}
};

جماعت بانٹو_Var_Std_راس final : عوامی بانٹو_گرہ_راس {
    VL_RTTI_IMPL(بانٹو_Var_Std_راس, بانٹو_گرہ_راس)

    std::string dotColor() ثابت override { واپس "skyblue"; }

عوامی:
    بانٹو_Var_Std_راس(V3_گراف* graphp, AstVarScope* vscp)
        : بانٹو_گرہ_راس{graphp, vscp} {}
};

جماعت بانٹو_Var_Post_راس final : عوامی بانٹو_گرہ_راس {
    VL_RTTI_IMPL(بانٹو_Var_Post_راس, بانٹو_گرہ_راس)

    std::string name() ثابت override { واپس "POST "s + بانٹو_گرہ_راس::name(); }
    std::string dotColor() ثابت override { واپس "CadetBlue"; }

عوامی:
    بانٹو_Var_Post_راس(V3_گراف* graphp, AstVarScope* vscp)
        : بانٹو_گرہ_راس{graphp, vscp} {}
};

جماعت بانٹو_Visitor final : عوامی VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> SplitVarStdVertex*: Regular program-flow variable vertex
    // AstVarScope::user2p  -> SplitVarPostVertex*: NBA written delayed variable vertex
    // Ast{StmtIsh}::user1p -> SplitStmtVertex*

    // NODE STATE
    // AstAlways::user3     -> bool: Block created by splitting, needs no further splitting
    ثابت VNUser3_اندر_Use m_inuser3;

    // STATE
    V3_گراف* m_graphp = خالی_اشاریہ;  // Dependency graph to analyze statement connectivity
    std::سمتیہ<بانٹو_Stmt_راس*> m_stmtStackps;  // Current statements being tracked
    بانٹو_Impure_راس* m_impureVtxp = خالی_اشاریہ;  // Vertex connecting impure statements
    ثابت حرف* m_no_بانٹو_Why = خالی_اشاریہ;  // Reason current block cannot be split
    منطقی m_اندر_Dly = جھوٹ;  // Inside AstAssignDly Lhs
    ثابت AstIf* m_currIfp = خالی_اشاریہ;  // The AstIf whose condition is currently visited
    VDouble0 m_statSplits;  // Statistic tracking

    // METHODS
    خالی جوڑو_کنارہ(V3_گراف_راس* fromp, V3_گراف_راس* چوٹی) {
        نیا V3_گراف_کنارہ{m_graphp, fromp, چوٹی, 1};
    }

    // Iterate the given list of statements, building the dependency graph
    خالی جائزہ_بلاک(Ast_گرہ* stmtsp) {
        اگر (m_no_بانٹو_Why) واپس;
        برائے (Ast_گرہ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Skip comments. They have no dependencies at all, so would always
            // form an independent component, and hence a split block, of their
            // own, which would be subsequently deleted as it does nothing.
            اگر (VN_IS(stmtp, Comment)) جاری;
            UASSERT_OBJ(!stmtp->user1p(), stmtp, "user1p should not be set");
            بانٹو_Stmt_راس* ثابت vtxp = نیا بانٹو_Stmt_راس{m_graphp, stmtp};
            stmtp->user1p(vtxp);
            m_stmtStackps.ڈالو_عقب(vtxp);
            iterate(stmtp);
            m_stmtStackps.نکالو_عقب();
        }
    }

    // Remove unnecessary edges, then color to find weakly connected components
    uint32_t color_Always_گراف() {
        اگر (dump_گراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_built", جھوٹ);

        // Prune duplicate edges. Not necessary for correctness, but simplifies dumps.
        m_graphp->remove_Redundant_کنارے_Max(&V3_گراف_کنارہ::followAlwaysTrue);
        اگر (dump_گراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nodup", جھوٹ);

        // Remove variable vertices that are not written in the block.
        // These are input-only to the block, so carry no dependency.
        برائے (V3_گراف_راس* ثابت vtxp : m_graphp->vertices().unlinkable()) {
            بانٹو_Var_Std_راس* ثابت vstdp = vtxp->cast<بانٹو_Var_Std_راس>();
            اگر (!vstdp || !vstdp->برآمد_خالی_خانے()) جاری;
            UINFOTREE(9, vstdp->nodep(), "", "Will remove deps on block input var:");
            vstdp->nodep()->user1p(خالی_اشاریہ);  // Don't leave a dangling pointer behind
            VL_DO_DANGLING(vstdp->unlinkDelete(m_graphp), vstdp);
        }
        اگر (dump_گراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_noinputs", جھوٹ);

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
        برائے (V3_گراف_راس* ثابت vtxp : m_graphp->vertices().unlinkable()) {
            بانٹو_Stmt_راس* ثابت stmtVtxp = vtxp->cast<بانٹو_Stmt_راس>();
            اگر (!stmtVtxp || !VN_IS(stmtVtxp->nodep(), If)) جاری;
            // Can't remove if dependent on a variable written in the block
            اگر (!stmtVtxp->برآمد_خالی_خانے()) جاری;
            // Depends only on block inputs, so can be split. Remove the vertex.
            stmtVtxp->nodep()->user1p(خالی_اشاریہ);
            stmtVtxp->unlinkDelete(m_graphp);
        }
        اگر (dump_گراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nofreeifs", جھوٹ);

        // Weak coloring to determine what must stay together in a single block
        ثابت uint32_t عدد_Colors = m_graphp->weaklyConnected(&V3_گراف_کنارہ::followAlwaysTrue);
        اگر (dump_گراف_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_colored", جھوٹ);
        واپس عدد_Colors;
    }

    // Take the statements of the given list, and return them distributed into one list per color.
    ساکن std::سمتیہ<Ast_گرہ*> بانٹو_Statements(Ast_گرہ* stmtsp, uint32_t عدد_Colors) {
        std::سمتیہ<Ast_گرہ*> نتیجہ{عدد_Colors, خالی_اشاریہ};
        برائے (Ast_گرہ *stmtp = stmtsp, *nextp = خالی_اشاریہ; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();  // 'stmtp' is unlinked below

            // Comments are dropped if the block is split
            اگر (VN_IS(stmtp, Comment)) جاری;

            // Pick up the statement vertex, which might be nullptr after pruning during analysis
            ثابت بانٹو_Stmt_راس* ثابت vtxp = stmtp->user1u().مقصد<بانٹو_Stmt_راس*>();

            // If statements are duplicated for each split branch
            اگر (AstIf* ثابت ifp = VN_CAST(stmtp, If)) {
                ثابت خودکار thens = بانٹو_Statements(ifp->thensp(), عدد_Colors);
                ثابت خودکار elses = بانٹو_Statements(ifp->elsesp(), عدد_Colors);
                File_سطر* ثابت flp = ifp->fileline();
                // Rebuild the 'if' for each color present in either branch
                منطقی خالی_خانے = سچ;
                برائے (uint32_t color = 0; color < عدد_Colors; ++color) {
                    اگر (!thens[color] && !elses[color]) جاری;
                    خالی_خانے = جھوٹ;
                    // The condition is cloned for each color. An impure condition
                    // keeps the 'if' and all it holds in one component, so just once.
                    AstIf* ثابت clonep = نیا AstIf{flp, ifp->condp()->clone_درخت(سچ),
                                                    thens[color], elses[color]};
                    // Preserve pragmas from unique if's so assertions work properly
                    clonep->uniquePragma(ifp->uniquePragma());
                    clonep->unique0Pragma(ifp->unique0Pragma());
                    clonep->ترجیح_Pragma(ifp->ترجیح_Pragma());
                    نتیجہ[color] = Ast_گرہ::جوڑو_اگلا(نتیجہ[color], clonep);
                }
                // There is nothing under the 'if' to guard. If its vertex was
                // removed as having no dependencies at all, then its
                // condition reads only block inputs and is pure, so the whole
                // 'if' can go. Otherwise the condition might have a side
                // effect, so keep just the condition, evaluated as a
                // statement, under the color of the 'if' itself.
                اگر (خالی_خانے && vtxp) {
                    ثابت uint32_t color = vtxp->color();
                    Ast_گرہ_اظہاریہ* ثابت condp = ifp->condp();
                    condp->unlink_Fr_عقب();
                    نتیجہ[color] = Ast_گرہ::جوڑو_اگلا(نتیجہ[color], نیا Ast_Stmt_اظہاریہ{flp, condp});
                }
                جاری;
            }

            // Move the leaf into its color's list
            ثابت uint32_t color = vtxp->color();
            نتیجہ[color] = Ast_گرہ::جوڑو_اگلا(نتیجہ[color], stmtp->unlink_Fr_عقب());
        }
        واپس نتیجہ;
    }

    // VISITORS
    خالی دورہ(AstAlways* nodep) override {
        // Skip blocks created below
        اگر (nodep->user3()) واپس;

        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_no_بانٹو_Why);
        VL_RESTORER(m_اندر_Dly);
        V3_گراف گراف;
        m_graphp = &گراف;
        m_impureVtxp = خالی_اشاریہ;
        m_no_بانٹو_Why = خالی_اشاریہ;
        m_اندر_Dly = جھوٹ;
        UASSERT_OBJ(m_stmtStackps.خالی_خانے(), nodep, "Statement stack not empty");

        // Build the graph
        ثابت VNUser1_اندر_Use user1_اندر_Use;
        ثابت VNUser2_اندر_Use user2_اندر_Use;
        جائزہ_بلاک(nodep->stmtsp());

        // We might have to give up
        اگر (m_no_بانٹو_Why) {
            UINFO(9, "  NoSplitBlock because " << m_no_بانٹو_Why);
            واپس;
        }

        // Color the graph to identify separable statements
        ثابت uint32_t عدد_Colors = color_Always_گراف();
        // If the whole block is one component (or empty), then nothing to split
        اگر (عدد_Colors <= 1) واپس;

        UINFO(6, "  splitting always " << nodep);

        // Count the number of new blocks inserted into the Ast: '1 -> n' split, so 'n - 1' extra
        m_statSplits += عدد_Colors - 1;

        // Unpick the statements out of the original block, into one list per color
        ثابت خودکار lists = بانٹو_Statements(nodep->stmtsp(), عدد_Colors);
        UASSERT_OBJ(lists.سائز() == عدد_Colors, nodep, "Inconsistent split");

        // Whatever 'splitStatements' did not take, (comments, empty ifs) is not needed any more
        اگر (Ast_گرہ* ثابت restp = nodep->stmtsp()) {
            VL_DO_DANGLING(restp->unlink_Fr_عقب_With_اگلا()->delete_درخت(), restp);
        }

        // Every color has a statement in it. Reuse the original block for the
        // first color, and add a new block after it for each of the rest.
        UASSERT_OBJ(lists.front(), nodep, "Color with no statements");
        nodep->جوڑو_Stmtsp(lists.front());
        Ast_گرہ* lastp = nodep;
        File_سطر* ثابت flp = nodep->fileline();
        ثابت VAlwaysKwd kwd = nodep->keyword();
        برائے (سائز_t i = 1; i < عدد_Colors; ++i) {
            Ast_گرہ* ثابت stmtsp = lists[i];
            UASSERT_OBJ(stmtsp, nodep, "Color with no statements");
            AstAlways* ثابت newp = نیا AstAlways{flp, kwd, خالی_اشاریہ, stmtsp};
            newp->user3(1);  // Do not split again
            lastp->جوڑو_اگلا_یہاں(newp);
            lastp = newp;
        }
    }

    خالی دورہ(AstIf* nodep) override {
        اگر (!m_graphp || m_no_بانٹو_Why) واپس;
        {
            VL_RESTORER(m_currIfp);
            m_currIfp = nodep;
            iterate_And_اگلا_Null(nodep->condp());
        }
        جائزہ_بلاک(nodep->thensp());
        جائزہ_بلاک(nodep->elsesp());
    }

    خالی دورہ(Ast_اظہاریہ_Stmt* nodep) override {
        اگر (!m_graphp || m_no_بانٹو_Why) واپس;
        VL_RESTORER(m_اندر_Dly);
        m_اندر_Dly = جھوٹ;
        iterateChildren(nodep);
    }

    خالی دورہ(Ast_تفویض_Dly* nodep) override {
        اگر (!m_graphp || m_no_بانٹو_Why) واپس;
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
        اگر (m_graphp) m_no_بانٹو_Why = "Handle";
        دورہ(static_cast<Ast_گرہ*>(nodep));
    }

    خالی دورہ(AstJumpGo*) override {
        اگر (!m_graphp || m_no_بانٹو_Why) واپس;
        m_no_بانٹو_Why = "JumpGo";
    }

    خالی دورہ(AstVarRef* nodep) override {
        اگر (!m_graphp || m_no_بانٹو_Why) واپس;
        UASSERT_OBJ(!m_stmtStackps.خالی_خانے(), nodep, "Not under a statement");

        // Constant lookups can be ignored
        اگر (nodep->varp()->isConst()) واپس;

        AstVarScope* ثابت vscp = nodep->varScopep();

        // SPEEDUP: We add duplicate edges, that should be fixed
        اگر (m_اندر_Dly && nodep->access().isWriteOrRW()) {
            // Delayed variable: is different from non-delayed variable, writes to it
            // are not observable while executing this block (NBA not yet committed),
            // so add only a write edge to a separate 'post' vertex.
            اگر (!vscp->user2p()) vscp->user2p(نیا بانٹو_Var_Post_راس{m_graphp, vscp});
            بانٹو_Var_Post_راس* ثابت vpostp = vscp->user2u().مقصد<بانٹو_Var_Post_راس*>();
            برائے (بانٹو_Stmt_راس* ثابت vtxp : m_stmtStackps) جوڑو_کنارہ(vpostp, vtxp);
        } ورنہ اگر (nodep->access().isWriteOrRW()) {
            // Regular (non-blocking) write: Need to maintain program-flow order
            اگر (!vscp->user1p()) vscp->user1p(نیا بانٹو_Var_Std_راس{m_graphp, vscp});
            بانٹو_Var_Std_راس* ثابت vstdp = vscp->user1u().مقصد<بانٹو_Var_Std_راس*>();
            برائے (بانٹو_Stmt_راس* ثابت vtxp : m_stmtStackps) جوڑو_کنارہ(vstdp, vtxp);
        } ورنہ {
            // Regular (non-blocking) read: Need to maintain program-flow order
            اگر (!vscp->user1p()) vscp->user1p(نیا بانٹو_Var_Std_راس{m_graphp, vscp});
            بانٹو_Var_Std_راس* ثابت vstdp = vscp->user1u().مقصد<بانٹو_Var_Std_راس*>();
            برائے (بانٹو_Stmt_راس* ثابت vtxp : m_stmtStackps) {
                // If this is an if statement it only depends on refs in its
                // own condition only (not those in its branches). For other
                // statements, just record the referene as normal.
                اگر (ثابت AstIf* ثابت ifp = VN_CAST(vtxp->nodep(), If)) {
                    اگر (ifp != m_currIfp) جاری;
                }
                جوڑو_کنارہ(vtxp, vstdp);
            }
        }
    }

    خالی دورہ(Ast_گرہ* nodep) override {
        // Outside AstAlways, just descend
        اگر (!m_graphp) {
            iterateChildren(nodep);
            واپس;
        }
        // Early exit if decided not to split
        اگر (m_no_بانٹو_Why) واپس;

        UASSERT_OBJ(!m_stmtStackps.خالی_خانے(), nodep, "Not under a statement");

        // Timing control prevents splitting
        اگر (nodep->isTimingControl()) {
            m_no_بانٹو_Why = "TimingControl";
            واپس;
        }

        // All impure statements must be grouped together.
        اگر (!nodep->is_خالص()) {
            اگر (!m_impureVtxp) m_impureVtxp = نیا بانٹو_Impure_راس{m_graphp, nodep};
            // One edge is enough to find the weakly connected components, but
            // it must point at the impure vertex, so it is an out edge (input
            // dependency) of any enclosing 'if' to prevent pruning.
            برائے (بانٹو_Stmt_راس* ثابت vtxp : m_stmtStackps) جوڑو_کنارہ(vtxp, m_impureVtxp);
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
    صریح بانٹو_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~بانٹو_Visitor() override { V3Stats::جوڑو_Stat("Optimizations, Split always", m_statSplits); }
    VL_UNCOPYABLE(بانٹو_Visitor);

عوامی:
    ساکن خالی apply(AstNetlist* nodep) { بانٹو_Visitor{nodep}; }
};

//######################################################################
// Split class functions

خالی V3_بانٹو::بانٹو_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    بانٹو_Visitor::apply(nodep);
    V3Global::dump_Check_Global_درخت("split", 0, dump_درخت_Either_Level() >= 3);
}
