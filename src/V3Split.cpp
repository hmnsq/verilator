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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Split.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <string>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Support classes

ਵਰਗ ਵੰਡੋ_ਗੰਢ_ਸ਼ੀਰਸ਼ VL_NOT_FINAL : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(ਵੰਡੋ_ਗੰਢ_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
    Ast_ਗੰਢ* ਅਚਲ m_nodep;

ਸੁਰੱਖਿਅਤ:
    ਵੰਡੋ_ਗੰਢ_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, Ast_ਗੰਢ* nodep)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp}
        , m_nodep{nodep} {}
    // ACCESSORS
    std::string name() ਅਚਲ override {
        ਵਾਪਸ cvt_ਮੰਜ਼ਿਲ_Hex(m_nodep) + ' ' + m_nodep->prettyTypeName();
    }

ਜਨਤਕ:
    Ast_ਗੰਢ* nodep() ਅਚਲ { ਵਾਪਸ m_nodep; }
};

ਵਰਗ ਵੰਡੋ_Impure_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ ਵੰਡੋ_ਗੰਢ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(ਵੰਡੋ_Impure_ਸ਼ੀਰਸ਼, ਵੰਡੋ_ਗੰਢ_ਸ਼ੀਰਸ਼)

    std::string name() ਅਚਲ override { ਵਾਪਸ "*IMPURE*"; }
    std::string dotColor() ਅਚਲ override { ਵਾਪਸ "green"; }

ਜਨਤਕ:
    ਸਪਸ਼ਟ ਵੰਡੋ_Impure_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, Ast_ਗੰਢ* nodep)
        : ਵੰਡੋ_ਗੰਢ_ਸ਼ੀਰਸ਼{graphp, nodep} {}
};

ਵਰਗ ਵੰਡੋ_Stmt_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ ਵੰਡੋ_ਗੰਢ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(ਵੰਡੋ_Stmt_ਸ਼ੀਰਸ਼, ਵੰਡੋ_ਗੰਢ_ਸ਼ੀਰਸ਼)

    std::string dotColor() ਅਚਲ override { ਵਾਪਸ "yellow"; }

ਜਨਤਕ:
    ਵੰਡੋ_Stmt_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, Ast_ਗੰਢ* nodep)
        : ਵੰਡੋ_ਗੰਢ_ਸ਼ੀਰਸ਼{graphp, nodep} {}
};

ਵਰਗ ਵੰਡੋ_Var_Std_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ ਵੰਡੋ_ਗੰਢ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(ਵੰਡੋ_Var_Std_ਸ਼ੀਰਸ਼, ਵੰਡੋ_ਗੰਢ_ਸ਼ੀਰਸ਼)

    std::string dotColor() ਅਚਲ override { ਵਾਪਸ "skyblue"; }

ਜਨਤਕ:
    ਵੰਡੋ_Var_Std_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, AstVarScope* vscp)
        : ਵੰਡੋ_ਗੰਢ_ਸ਼ੀਰਸ਼{graphp, vscp} {}
};

ਵਰਗ ਵੰਡੋ_Var_Post_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ ਵੰਡੋ_ਗੰਢ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(ਵੰਡੋ_Var_Post_ਸ਼ੀਰਸ਼, ਵੰਡੋ_ਗੰਢ_ਸ਼ੀਰਸ਼)

    std::string name() ਅਚਲ override { ਵਾਪਸ "POST "s + ਵੰਡੋ_ਗੰਢ_ਸ਼ੀਰਸ਼::name(); }
    std::string dotColor() ਅਚਲ override { ਵਾਪਸ "CadetBlue"; }

ਜਨਤਕ:
    ਵੰਡੋ_Var_Post_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, AstVarScope* vscp)
        : ਵੰਡੋ_ਗੰਢ_ਸ਼ੀਰਸ਼{graphp, vscp} {}
};

ਵਰਗ ਵੰਡੋ_Visitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> SplitVarStdVertex*: Regular program-flow variable vertex
    // AstVarScope::user2p  -> SplitVarPostVertex*: NBA written delayed variable vertex
    // Ast{StmtIsh}::user1p -> SplitStmtVertex*

    // NODE STATE
    // AstAlways::user3     -> bool: Block created by splitting, needs no further splitting
    ਅਚਲ VNUser3_ਵਿੱਚ_Use m_inuser3;

    // STATE
    V3_ਗ੍ਰਾਫ* m_graphp = ਖਾਲੀ_ਸੂਚਕ;  // Dependency graph to analyze statement connectivity
    std::ਸਦਿਸ਼<ਵੰਡੋ_Stmt_ਸ਼ੀਰਸ਼*> m_stmtStackps;  // Current statements being tracked
    ਵੰਡੋ_Impure_ਸ਼ੀਰਸ਼* m_impureVtxp = ਖਾਲੀ_ਸੂਚਕ;  // Vertex connecting impure statements
    ਅਚਲ ਅੱਖਰ* m_no_ਵੰਡੋ_Why = ਖਾਲੀ_ਸੂਚਕ;  // Reason current block cannot be split
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Dly = ਝੂਠ;  // Inside AstAssignDly Lhs
    ਅਚਲ AstIf* m_currIfp = ਖਾਲੀ_ਸੂਚਕ;  // The AstIf whose condition is currently visited
    VDouble0 m_statSplits;  // Statistic tracking

    // METHODS
    ਖਾਲੀ ਜੋੜੋ_ਕਿਨਾਰਾ(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ) {
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{m_graphp, fromp, ਸਿਖਰ, 1};
    }

    // Iterate the given list of statements, building the dependency graph
    ਖਾਲੀ ਜਾਂਚ_ਖੰਡ(Ast_ਗੰਢ* stmtsp) {
        ਜੇ (m_no_ਵੰਡੋ_Why) ਵਾਪਸ;
        ਲਈ (Ast_ਗੰਢ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Skip comments. They have no dependencies at all, so would always
            // form an independent component, and hence a split block, of their
            // own, which would be subsequently deleted as it does nothing.
            ਜੇ (VN_IS(stmtp, Comment)) ਜਾਰੀ;
            UASSERT_OBJ(!stmtp->user1p(), stmtp, "user1p should not be set");
            ਵੰਡੋ_Stmt_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = ਨਵਾਂ ਵੰਡੋ_Stmt_ਸ਼ੀਰਸ਼{m_graphp, stmtp};
            stmtp->user1p(vtxp);
            m_stmtStackps.ਧੱਕੋ_ਪਿੱਠ(vtxp);
            iterate(stmtp);
            m_stmtStackps.ਕੱਢੋ_ਪਿੱਠ();
        }
    }

    // Remove unnecessary edges, then color to find weakly connected components
    uint32_t color_Always_ਗ੍ਰਾਫ() {
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_built", ਝੂਠ);

        // Prune duplicate edges. Not necessary for correctness, but simplifies dumps.
        m_graphp->remove_Redundant_ਕਿਨਾਰੇ_Max(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nodup", ਝੂਠ);

        // Remove variable vertices that are not written in the block.
        // These are input-only to the block, so carry no dependency.
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_graphp->vertices().unlinkable()) {
            ਵੰਡੋ_Var_Std_ਸ਼ੀਰਸ਼* ਅਚਲ vstdp = vtxp->cast<ਵੰਡੋ_Var_Std_ਸ਼ੀਰਸ਼>();
            ਜੇ (!vstdp || !vstdp->ਬਾਹਰਲਾ_ਖਾਲੀ_ਥਾਵਾਂ()) ਜਾਰੀ;
            UINFOTREE(9, vstdp->nodep(), "", "Will remove deps on block input var:");
            vstdp->nodep()->user1p(ਖਾਲੀ_ਸੂਚਕ);  // Don't leave a dangling pointer behind
            VL_DO_DANGLING(vstdp->unlinkDelete(m_graphp), vstdp);
        }
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_noinputs", ਝੂਠ);

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
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_graphp->vertices().unlinkable()) {
            ਵੰਡੋ_Stmt_ਸ਼ੀਰਸ਼* ਅਚਲ stmtVtxp = vtxp->cast<ਵੰਡੋ_Stmt_ਸ਼ੀਰਸ਼>();
            ਜੇ (!stmtVtxp || !VN_IS(stmtVtxp->nodep(), If)) ਜਾਰੀ;
            // Can't remove if dependent on a variable written in the block
            ਜੇ (!stmtVtxp->ਬਾਹਰਲਾ_ਖਾਲੀ_ਥਾਵਾਂ()) ਜਾਰੀ;
            // Depends only on block inputs, so can be split. Remove the vertex.
            stmtVtxp->nodep()->user1p(ਖਾਲੀ_ਸੂਚਕ);
            stmtVtxp->unlinkDelete(m_graphp);
        }
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nofreeifs", ਝੂਠ);

        // Weak coloring to determine what must stay together in a single block
        ਅਚਲ uint32_t ਅੰਕ_Colors = m_graphp->weaklyConnected(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_colored", ਝੂਠ);
        ਵਾਪਸ ਅੰਕ_Colors;
    }

    // Take the statements of the given list, and return them distributed into one list per color.
    ਸਥਿਰ std::ਸਦਿਸ਼<Ast_ਗੰਢ*> ਵੰਡੋ_Statements(Ast_ਗੰਢ* stmtsp, uint32_t ਅੰਕ_Colors) {
        std::ਸਦਿਸ਼<Ast_ਗੰਢ*> ਨਤੀਜਾ{ਅੰਕ_Colors, ਖਾਲੀ_ਸੂਚਕ};
        ਲਈ (Ast_ਗੰਢ *stmtp = stmtsp, *nextp = ਖਾਲੀ_ਸੂਚਕ; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();  // 'stmtp' is unlinked below

            // Comments are dropped if the block is split
            ਜੇ (VN_IS(stmtp, Comment)) ਜਾਰੀ;

            // Pick up the statement vertex, which might be nullptr after pruning during analysis
            ਅਚਲ ਵੰਡੋ_Stmt_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = stmtp->user1u().ਮੰਜ਼ਿਲ<ਵੰਡੋ_Stmt_ਸ਼ੀਰਸ਼*>();

            // If statements are duplicated for each split branch
            ਜੇ (AstIf* ਅਚਲ ifp = VN_CAST(stmtp, If)) {
                ਅਚਲ ਸਵੈਚਾਲਿਤ thens = ਵੰਡੋ_Statements(ifp->thensp(), ਅੰਕ_Colors);
                ਅਚਲ ਸਵੈਚਾਲਿਤ elses = ਵੰਡੋ_Statements(ifp->elsesp(), ਅੰਕ_Colors);
                File_ਸਤਰ* ਅਚਲ flp = ifp->fileline();
                // Rebuild the 'if' for each color present in either branch
                ਤਾਰਕਿਕ ਖਾਲੀ_ਥਾਵਾਂ = ਸੱਚ;
                ਲਈ (uint32_t color = 0; color < ਅੰਕ_Colors; ++color) {
                    ਜੇ (!thens[color] && !elses[color]) ਜਾਰੀ;
                    ਖਾਲੀ_ਥਾਵਾਂ = ਝੂਠ;
                    // The condition is cloned for each color. An impure condition
                    // keeps the 'if' and all it holds in one component, so just once.
                    AstIf* ਅਚਲ clonep = ਨਵਾਂ AstIf{flp, ifp->condp()->clone_ਰੁੱਖ(ਸੱਚ),
                                                    thens[color], elses[color]};
                    // Preserve pragmas from unique if's so assertions work properly
                    clonep->uniquePragma(ifp->uniquePragma());
                    clonep->unique0Pragma(ifp->unique0Pragma());
                    clonep->ਤਰਜੀਹ_Pragma(ifp->ਤਰਜੀਹ_Pragma());
                    ਨਤੀਜਾ[color] = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(ਨਤੀਜਾ[color], clonep);
                }
                // There is nothing under the 'if' to guard. If its vertex was
                // removed as having no dependencies at all, then its
                // condition reads only block inputs and is pure, so the whole
                // 'if' can go. Otherwise the condition might have a side
                // effect, so keep just the condition, evaluated as a
                // statement, under the color of the 'if' itself.
                ਜੇ (ਖਾਲੀ_ਥਾਵਾਂ && vtxp) {
                    ਅਚਲ uint32_t color = vtxp->color();
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = ifp->condp();
                    condp->unlink_Fr_ਪਿੱਠ();
                    ਨਤੀਜਾ[color] = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(ਨਤੀਜਾ[color], ਨਵਾਂ Ast_Stmt_ਵਿਅੰਜਕ{flp, condp});
                }
                ਜਾਰੀ;
            }

            // Move the leaf into its color's list
            ਅਚਲ uint32_t color = vtxp->color();
            ਨਤੀਜਾ[color] = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(ਨਤੀਜਾ[color], stmtp->unlink_Fr_ਪਿੱਠ());
        }
        ਵਾਪਸ ਨਤੀਜਾ;
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstAlways* nodep) override {
        // Skip blocks created below
        ਜੇ (nodep->user3()) ਵਾਪਸ;

        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_no_ਵੰਡੋ_Why);
        VL_RESTORER(m_ਵਿੱਚ_Dly);
        V3_ਗ੍ਰਾਫ ਗ੍ਰਾਫ;
        m_graphp = &ਗ੍ਰਾਫ;
        m_impureVtxp = ਖਾਲੀ_ਸੂਚਕ;
        m_no_ਵੰਡੋ_Why = ਖਾਲੀ_ਸੂਚਕ;
        m_ਵਿੱਚ_Dly = ਝੂਠ;
        UASSERT_OBJ(m_stmtStackps.ਖਾਲੀ_ਥਾਵਾਂ(), nodep, "Statement stack not empty");

        // Build the graph
        ਅਚਲ VNUser1_ਵਿੱਚ_Use user1_ਵਿੱਚ_Use;
        ਅਚਲ VNUser2_ਵਿੱਚ_Use user2_ਵਿੱਚ_Use;
        ਜਾਂਚ_ਖੰਡ(nodep->stmtsp());

        // We might have to give up
        ਜੇ (m_no_ਵੰਡੋ_Why) {
            UINFO(9, "  NoSplitBlock because " << m_no_ਵੰਡੋ_Why);
            ਵਾਪਸ;
        }

        // Color the graph to identify separable statements
        ਅਚਲ uint32_t ਅੰਕ_Colors = color_Always_ਗ੍ਰਾਫ();
        // If the whole block is one component (or empty), then nothing to split
        ਜੇ (ਅੰਕ_Colors <= 1) ਵਾਪਸ;

        UINFO(6, "  splitting always " << nodep);

        // Count the number of new blocks inserted into the Ast: '1 -> n' split, so 'n - 1' extra
        m_statSplits += ਅੰਕ_Colors - 1;

        // Unpick the statements out of the original block, into one list per color
        ਅਚਲ ਸਵੈਚਾਲਿਤ lists = ਵੰਡੋ_Statements(nodep->stmtsp(), ਅੰਕ_Colors);
        UASSERT_OBJ(lists.ਮਾਪ() == ਅੰਕ_Colors, nodep, "Inconsistent split");

        // Whatever 'splitStatements' did not take, (comments, empty ifs) is not needed any more
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ restp = nodep->stmtsp()) {
            VL_DO_DANGLING(restp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ()->delete_ਰੁੱਖ(), restp);
        }

        // Every color has a statement in it. Reuse the original block for the
        // first color, and add a new block after it for each of the rest.
        UASSERT_OBJ(lists.front(), nodep, "Color with no statements");
        nodep->ਜੋੜੋ_Stmtsp(lists.front());
        Ast_ਗੰਢ* lastp = nodep;
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        ਅਚਲ VAlwaysKwd kwd = nodep->keyword();
        ਲਈ (ਮਾਪ_t i = 1; i < ਅੰਕ_Colors; ++i) {
            Ast_ਗੰਢ* ਅਚਲ stmtsp = lists[i];
            UASSERT_OBJ(stmtsp, nodep, "Color with no statements");
            AstAlways* ਅਚਲ newp = ਨਵਾਂ AstAlways{flp, kwd, ਖਾਲੀ_ਸੂਚਕ, stmtsp};
            newp->user3(1);  // Do not split again
            lastp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(newp);
            lastp = newp;
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstIf* nodep) override {
        ਜੇ (!m_graphp || m_no_ਵੰਡੋ_Why) ਵਾਪਸ;
        {
            VL_RESTORER(m_currIfp);
            m_currIfp = nodep;
            iterate_And_ਅਗਲਾ_Null(nodep->condp());
        }
        ਜਾਂਚ_ਖੰਡ(nodep->thensp());
        ਜਾਂਚ_ਖੰਡ(nodep->elsesp());
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਵਿਅੰਜਕ_Stmt* nodep) override {
        ਜੇ (!m_graphp || m_no_ਵੰਡੋ_Why) ਵਾਪਸ;
        VL_RESTORER(m_ਵਿੱਚ_Dly);
        m_ਵਿੱਚ_Dly = ਝੂਠ;
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Dly* nodep) override {
        ਜੇ (!m_graphp || m_no_ਵੰਡੋ_Why) ਵਾਪਸ;
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
        ਜੇ (m_graphp) m_no_ਵੰਡੋ_Why = "Handle";
        ਦੇਖੋ(static_cast<Ast_ਗੰਢ*>(nodep));
    }

    ਖਾਲੀ ਦੇਖੋ(AstJumpGo*) override {
        ਜੇ (!m_graphp || m_no_ਵੰਡੋ_Why) ਵਾਪਸ;
        m_no_ਵੰਡੋ_Why = "JumpGo";
    }

    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        ਜੇ (!m_graphp || m_no_ਵੰਡੋ_Why) ਵਾਪਸ;
        UASSERT_OBJ(!m_stmtStackps.ਖਾਲੀ_ਥਾਵਾਂ(), nodep, "Not under a statement");

        // Constant lookups can be ignored
        ਜੇ (nodep->varp()->isConst()) ਵਾਪਸ;

        AstVarScope* ਅਚਲ vscp = nodep->varScopep();

        // SPEEDUP: We add duplicate edges, that should be fixed
        ਜੇ (m_ਵਿੱਚ_Dly && nodep->access().isWriteOrRW()) {
            // Delayed variable: is different from non-delayed variable, writes to it
            // are not observable while executing this block (NBA not yet committed),
            // so add only a write edge to a separate 'post' vertex.
            ਜੇ (!vscp->user2p()) vscp->user2p(ਨਵਾਂ ਵੰਡੋ_Var_Post_ਸ਼ੀਰਸ਼{m_graphp, vscp});
            ਵੰਡੋ_Var_Post_ਸ਼ੀਰਸ਼* ਅਚਲ vpostp = vscp->user2u().ਮੰਜ਼ਿਲ<ਵੰਡੋ_Var_Post_ਸ਼ੀਰਸ਼*>();
            ਲਈ (ਵੰਡੋ_Stmt_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_stmtStackps) ਜੋੜੋ_ਕਿਨਾਰਾ(vpostp, vtxp);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->access().isWriteOrRW()) {
            // Regular (non-blocking) write: Need to maintain program-flow order
            ਜੇ (!vscp->user1p()) vscp->user1p(ਨਵਾਂ ਵੰਡੋ_Var_Std_ਸ਼ੀਰਸ਼{m_graphp, vscp});
            ਵੰਡੋ_Var_Std_ਸ਼ੀਰਸ਼* ਅਚਲ vstdp = vscp->user1u().ਮੰਜ਼ਿਲ<ਵੰਡੋ_Var_Std_ਸ਼ੀਰਸ਼*>();
            ਲਈ (ਵੰਡੋ_Stmt_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_stmtStackps) ਜੋੜੋ_ਕਿਨਾਰਾ(vstdp, vtxp);
        } ਨਹੀਂਤਾਂ {
            // Regular (non-blocking) read: Need to maintain program-flow order
            ਜੇ (!vscp->user1p()) vscp->user1p(ਨਵਾਂ ਵੰਡੋ_Var_Std_ਸ਼ੀਰਸ਼{m_graphp, vscp});
            ਵੰਡੋ_Var_Std_ਸ਼ੀਰਸ਼* ਅਚਲ vstdp = vscp->user1u().ਮੰਜ਼ਿਲ<ਵੰਡੋ_Var_Std_ਸ਼ੀਰਸ਼*>();
            ਲਈ (ਵੰਡੋ_Stmt_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_stmtStackps) {
                // If this is an if statement it only depends on refs in its
                // own condition only (not those in its branches). For other
                // statements, just record the referene as normal.
                ਜੇ (ਅਚਲ AstIf* ਅਚਲ ifp = VN_CAST(vtxp->nodep(), If)) {
                    ਜੇ (ifp != m_currIfp) ਜਾਰੀ;
                }
                ਜੋੜੋ_ਕਿਨਾਰਾ(vtxp, vstdp);
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        // Outside AstAlways, just descend
        ਜੇ (!m_graphp) {
            iterateChildren(nodep);
            ਵਾਪਸ;
        }
        // Early exit if decided not to split
        ਜੇ (m_no_ਵੰਡੋ_Why) ਵਾਪਸ;

        UASSERT_OBJ(!m_stmtStackps.ਖਾਲੀ_ਥਾਵਾਂ(), nodep, "Not under a statement");

        // Timing control prevents splitting
        ਜੇ (nodep->isTimingControl()) {
            m_no_ਵੰਡੋ_Why = "TimingControl";
            ਵਾਪਸ;
        }

        // All impure statements must be grouped together.
        ਜੇ (!nodep->is_ਸ਼ੁੱਧ()) {
            ਜੇ (!m_impureVtxp) m_impureVtxp = ਨਵਾਂ ਵੰਡੋ_Impure_ਸ਼ੀਰਸ਼{m_graphp, nodep};
            // One edge is enough to find the weakly connected components, but
            // it must point at the impure vertex, so it is an out edge (input
            // dependency) of any enclosing 'if' to prevent pruning.
            ਲਈ (ਵੰਡੋ_Stmt_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_stmtStackps) ਜੋੜੋ_ਕਿਨਾਰਾ(vtxp, m_impureVtxp);
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
    ਸਪਸ਼ਟ ਵੰਡੋ_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~ਵੰਡੋ_Visitor() override { V3Stats::ਜੋੜੋ_Stat("Optimizations, Split always", m_statSplits); }
    VL_UNCOPYABLE(ਵੰਡੋ_Visitor);

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(AstNetlist* nodep) { ਵੰਡੋ_Visitor{nodep}; }
};

//######################################################################
// Split class functions

ਖਾਲੀ V3_ਵੰਡੋ::ਵੰਡੋ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    ਵੰਡੋ_Visitor::apply(nodep);
    V3Global::dump_Check_Global_ਰੁੱਖ("split", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
