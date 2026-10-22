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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Split.h"

#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <string>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Support classes

ವರ್ಗ ವಿಭಜಿಸು_ಗಂಟು_ಶೃಂಗ VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(ವಿಭಜಿಸು_ಗಂಟು_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ m_nodep;

ಸಂರಕ್ಷಿತ:
    ವಿಭಜಿಸು_ಗಂಟು_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, Ast_ಗಂಟು* nodep)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp}
        , m_nodep{nodep} {}
    // ACCESSORS
    std::string name() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ cvt_ಗಮ್ಯ_Hex(m_nodep) + ' ' + m_nodep->prettyTypeName();
    }

ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು* nodep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_nodep; }
};

ವರ್ಗ ವಿಭಜಿಸು_Impure_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ ವಿಭಜಿಸು_ಗಂಟು_ಶೃಂಗ {
    VL_RTTI_IMPL(ವಿಭಜಿಸು_Impure_ಶೃಂಗ, ವಿಭಜಿಸು_ಗಂಟು_ಶೃಂಗ)

    std::string name() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "*IMPURE*"; }
    std::string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "green"; }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ ವಿಭಜಿಸು_Impure_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, Ast_ಗಂಟು* nodep)
        : ವಿಭಜಿಸು_ಗಂಟು_ಶೃಂಗ{graphp, nodep} {}
};

ವರ್ಗ ವಿಭಜಿಸು_Stmt_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ ವಿಭಜಿಸು_ಗಂಟು_ಶೃಂಗ {
    VL_RTTI_IMPL(ವಿಭಜಿಸು_Stmt_ಶೃಂಗ, ವಿಭಜಿಸು_ಗಂಟು_ಶೃಂಗ)

    std::string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "yellow"; }

ಸಾರ್ವಜನಿಕ:
    ವಿಭಜಿಸು_Stmt_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, Ast_ಗಂಟು* nodep)
        : ವಿಭಜಿಸು_ಗಂಟು_ಶೃಂಗ{graphp, nodep} {}
};

ವರ್ಗ ವಿಭಜಿಸು_Var_Std_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ ವಿಭಜಿಸು_ಗಂಟು_ಶೃಂಗ {
    VL_RTTI_IMPL(ವಿಭಜಿಸು_Var_Std_ಶೃಂಗ, ವಿಭಜಿಸು_ಗಂಟು_ಶೃಂಗ)

    std::string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "skyblue"; }

ಸಾರ್ವಜನಿಕ:
    ವಿಭಜಿಸು_Var_Std_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, AstVarScope* vscp)
        : ವಿಭಜಿಸು_ಗಂಟು_ಶೃಂಗ{graphp, vscp} {}
};

ವರ್ಗ ವಿಭಜಿಸು_Var_Post_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ ವಿಭಜಿಸು_ಗಂಟು_ಶೃಂಗ {
    VL_RTTI_IMPL(ವಿಭಜಿಸು_Var_Post_ಶೃಂಗ, ವಿಭಜಿಸು_ಗಂಟು_ಶೃಂಗ)

    std::string name() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "POST "s + ವಿಭಜಿಸು_ಗಂಟು_ಶೃಂಗ::name(); }
    std::string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "CadetBlue"; }

ಸಾರ್ವಜನಿಕ:
    ವಿಭಜಿಸು_Var_Post_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, AstVarScope* vscp)
        : ವಿಭಜಿಸು_ಗಂಟು_ಶೃಂಗ{graphp, vscp} {}
};

ವರ್ಗ ವಿಭಜಿಸು_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> SplitVarStdVertex*: Regular program-flow variable vertex
    // AstVarScope::user2p  -> SplitVarPostVertex*: NBA written delayed variable vertex
    // Ast{StmtIsh}::user1p -> SplitStmtVertex*

    // NODE STATE
    // AstAlways::user3     -> bool: Block created by splitting, needs no further splitting
    ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use m_inuser3;

    // STATE
    V3_ನಕ್ಷೆ* m_graphp = ಶೂನ್ಯಸೂಚಕ;  // Dependency graph to analyze statement connectivity
    std::ಸದಿಶ<ವಿಭಜಿಸು_Stmt_ಶೃಂಗ*> m_stmtStackps;  // Current statements being tracked
    ವಿಭಜಿಸು_Impure_ಶೃಂಗ* m_impureVtxp = ಶೂನ್ಯಸೂಚಕ;  // Vertex connecting impure statements
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* m_no_ವಿಭಜಿಸು_Why = ಶೂನ್ಯಸೂಚಕ;  // Reason current block cannot be split
    ತಾರ್ಕಿಕ m_ಒಳ_Dly = ಸುಳ್ಳು;  // Inside AstAssignDly Lhs
    ಸ್ಥಿರಾಂಕ AstIf* m_currIfp = ಶೂನ್ಯಸೂಚಕ;  // The AstIf whose condition is currently visited
    VDouble0 m_statSplits;  // Statistic tracking

    // METHODS
    ಶೂನ್ಯ ಕೂಡಿಸು_ಅಂಚು(V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp, V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ) {
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{m_graphp, fromp, ತುದಿ, 1};
    }

    // Iterate the given list of statements, building the dependency graph
    ಶೂನ್ಯ ಪರಿಶೀಲನೆ_ಖಂಡ(Ast_ಗಂಟು* stmtsp) {
        ಆದರೆ (m_no_ವಿಭಜಿಸು_Why) ಹಿಂತಿರುಗಿ;
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Skip comments. They have no dependencies at all, so would always
            // form an independent component, and hence a split block, of their
            // own, which would be subsequently deleted as it does nothing.
            ಆದರೆ (VN_IS(stmtp, Comment)) ಮುಂದುವರಿ;
            UASSERT_OBJ(!stmtp->user1p(), stmtp, "user1p should not be set");
            ವಿಭಜಿಸು_Stmt_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = ಹೊಸ ವಿಭಜಿಸು_Stmt_ಶೃಂಗ{m_graphp, stmtp};
            stmtp->user1p(vtxp);
            m_stmtStackps.ತಳ್ಳು_ಹಿಂಭಾಗ(vtxp);
            iterate(stmtp);
            m_stmtStackps.ತೆಗೆ_ಹಿಂಭಾಗ();
        }
    }

    // Remove unnecessary edges, then color to find weakly connected components
    uint32_t color_Always_ನಕ್ಷೆ() {
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_built", ಸುಳ್ಳು);

        // Prune duplicate edges. Not necessary for correctness, but simplifies dumps.
        m_graphp->remove_Redundant_ಅಂಚುಗಳು_Max(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nodup", ಸುಳ್ಳು);

        // Remove variable vertices that are not written in the block.
        // These are input-only to the block, so carry no dependency.
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_graphp->vertices().unlinkable()) {
            ವಿಭಜಿಸು_Var_Std_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vstdp = vtxp->cast<ವಿಭಜಿಸು_Var_Std_ಶೃಂಗ>();
            ಆದರೆ (!vstdp || !vstdp->ಹೊರಹೋಗು_ಖಾಲಿಗಳು()) ಮುಂದುವರಿ;
            UINFOTREE(9, vstdp->nodep(), "", "Will remove deps on block input var:");
            vstdp->nodep()->user1p(ಶೂನ್ಯಸೂಚಕ);  // Don't leave a dangling pointer behind
            VL_DO_DANGLING(vstdp->unlinkDelete(m_graphp), vstdp);
        }
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_noinputs", ಸುಳ್ಳು);

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
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_graphp->vertices().unlinkable()) {
            ವಿಭಜಿಸು_Stmt_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ stmtVtxp = vtxp->cast<ವಿಭಜಿಸು_Stmt_ಶೃಂಗ>();
            ಆದರೆ (!stmtVtxp || !VN_IS(stmtVtxp->nodep(), If)) ಮುಂದುವರಿ;
            // Can't remove if dependent on a variable written in the block
            ಆದರೆ (!stmtVtxp->ಹೊರಹೋಗು_ಖಾಲಿಗಳು()) ಮುಂದುವರಿ;
            // Depends only on block inputs, so can be split. Remove the vertex.
            stmtVtxp->nodep()->user1p(ಶೂನ್ಯಸೂಚಕ);
            stmtVtxp->unlinkDelete(m_graphp);
        }
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nofreeifs", ಸುಳ್ಳು);

        // Weak coloring to determine what must stay together in a single block
        ಸ್ಥಿರಾಂಕ uint32_t ಅಂಕೆ_Colors = m_graphp->weaklyConnected(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_colored", ಸುಳ್ಳು);
        ಹಿಂತಿರುಗಿ ಅಂಕೆ_Colors;
    }

    // Take the statements of the given list, and return them distributed into one list per color.
    ಸ್ಥಿರ std::ಸದಿಶ<Ast_ಗಂಟು*> ವಿಭಜಿಸು_Statements(Ast_ಗಂಟು* stmtsp, uint32_t ಅಂಕೆ_Colors) {
        std::ಸದಿಶ<Ast_ಗಂಟು*> ಫಲಿತಾಂಶ{ಅಂಕೆ_Colors, ಶೂನ್ಯಸೂಚಕ};
        ಗಾಗಿ (Ast_ಗಂಟು *stmtp = stmtsp, *nextp = ಶೂನ್ಯಸೂಚಕ; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();  // 'stmtp' is unlinked below

            // Comments are dropped if the block is split
            ಆದರೆ (VN_IS(stmtp, Comment)) ಮುಂದುವರಿ;

            // Pick up the statement vertex, which might be nullptr after pruning during analysis
            ಸ್ಥಿರಾಂಕ ವಿಭಜಿಸು_Stmt_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = stmtp->user1u().ಗಮ್ಯ<ವಿಭಜಿಸು_Stmt_ಶೃಂಗ*>();

            // If statements are duplicated for each split branch
            ಆದರೆ (AstIf* ಸ್ಥಿರಾಂಕ ifp = VN_CAST(stmtp, If)) {
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ thens = ವಿಭಜಿಸು_Statements(ifp->thensp(), ಅಂಕೆ_Colors);
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ elses = ವಿಭಜಿಸು_Statements(ifp->elsesp(), ಅಂಕೆ_Colors);
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = ifp->fileline();
                // Rebuild the 'if' for each color present in either branch
                ತಾರ್ಕಿಕ ಖಾಲಿಗಳು = ನಿಜ;
                ಗಾಗಿ (uint32_t color = 0; color < ಅಂಕೆ_Colors; ++color) {
                    ಆದರೆ (!thens[color] && !elses[color]) ಮುಂದುವರಿ;
                    ಖಾಲಿಗಳು = ಸುಳ್ಳು;
                    // The condition is cloned for each color. An impure condition
                    // keeps the 'if' and all it holds in one component, so just once.
                    AstIf* ಸ್ಥಿರಾಂಕ clonep = ಹೊಸ AstIf{flp, ifp->condp()->clone_ವೃಕ್ಷ(ನಿಜ),
                                                    thens[color], elses[color]};
                    // Preserve pragmas from unique if's so assertions work properly
                    clonep->uniquePragma(ifp->uniquePragma());
                    clonep->unique0Pragma(ifp->unique0Pragma());
                    clonep->ಆದ್ಯತೆ_Pragma(ifp->ಆದ್ಯತೆ_Pragma());
                    ಫಲಿತಾಂಶ[color] = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(ಫಲಿತಾಂಶ[color], clonep);
                }
                // There is nothing under the 'if' to guard. If its vertex was
                // removed as having no dependencies at all, then its
                // condition reads only block inputs and is pure, so the whole
                // 'if' can go. Otherwise the condition might have a side
                // effect, so keep just the condition, evaluated as a
                // statement, under the color of the 'if' itself.
                ಆದರೆ (ಖಾಲಿಗಳು && vtxp) {
                    ಸ್ಥಿರಾಂಕ uint32_t color = vtxp->color();
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = ifp->condp();
                    condp->unlink_Fr_ಹಿಂಭಾಗ();
                    ಫಲಿತಾಂಶ[color] = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(ಫಲಿತಾಂಶ[color], ಹೊಸ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{flp, condp});
                }
                ಮುಂದುವರಿ;
            }

            // Move the leaf into its color's list
            ಸ್ಥಿರಾಂಕ uint32_t color = vtxp->color();
            ಫಲಿತಾಂಶ[color] = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(ಫಲಿತಾಂಶ[color], stmtp->unlink_Fr_ಹಿಂಭಾಗ());
        }
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstAlways* nodep) override {
        // Skip blocks created below
        ಆದರೆ (nodep->user3()) ಹಿಂತಿರುಗಿ;

        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_no_ವಿಭಜಿಸು_Why);
        VL_RESTORER(m_ಒಳ_Dly);
        V3_ನಕ್ಷೆ ನಕ್ಷೆ;
        m_graphp = &ನಕ್ಷೆ;
        m_impureVtxp = ಶೂನ್ಯಸೂಚಕ;
        m_no_ವಿಭಜಿಸು_Why = ಶೂನ್ಯಸೂಚಕ;
        m_ಒಳ_Dly = ಸುಳ್ಳು;
        UASSERT_OBJ(m_stmtStackps.ಖಾಲಿಗಳು(), nodep, "Statement stack not empty");

        // Build the graph
        ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use user1_ಒಳ_Use;
        ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use user2_ಒಳ_Use;
        ಪರಿಶೀಲನೆ_ಖಂಡ(nodep->stmtsp());

        // We might have to give up
        ಆದರೆ (m_no_ವಿಭಜಿಸು_Why) {
            UINFO(9, "  NoSplitBlock because " << m_no_ವಿಭಜಿಸು_Why);
            ಹಿಂತಿರುಗಿ;
        }

        // Color the graph to identify separable statements
        ಸ್ಥಿರಾಂಕ uint32_t ಅಂಕೆ_Colors = color_Always_ನಕ್ಷೆ();
        // If the whole block is one component (or empty), then nothing to split
        ಆದರೆ (ಅಂಕೆ_Colors <= 1) ಹಿಂತಿರುಗಿ;

        UINFO(6, "  splitting always " << nodep);

        // Count the number of new blocks inserted into the Ast: '1 -> n' split, so 'n - 1' extra
        m_statSplits += ಅಂಕೆ_Colors - 1;

        // Unpick the statements out of the original block, into one list per color
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ lists = ವಿಭಜಿಸು_Statements(nodep->stmtsp(), ಅಂಕೆ_Colors);
        UASSERT_OBJ(lists.ಗಾತ್ರ_2() == ಅಂಕೆ_Colors, nodep, "Inconsistent split");

        // Whatever 'splitStatements' did not take, (comments, empty ifs) is not needed any more
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ restp = nodep->stmtsp()) {
            VL_DO_DANGLING(restp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ()->delete_ವೃಕ್ಷ(), restp);
        }

        // Every color has a statement in it. Reuse the original block for the
        // first color, and add a new block after it for each of the rest.
        UASSERT_OBJ(lists.front(), nodep, "Color with no statements");
        nodep->ಕೂಡಿಸು_Stmtsp(lists.front());
        Ast_ಗಂಟು* lastp = nodep;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        ಸ್ಥಿರಾಂಕ VAlwaysKwd kwd = nodep->keyword();
        ಗಾಗಿ (ಗಾತ್ರ_t i = 1; i < ಅಂಕೆ_Colors; ++i) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = lists[i];
            UASSERT_OBJ(stmtsp, nodep, "Color with no statements");
            AstAlways* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstAlways{flp, kwd, ಶೂನ್ಯಸೂಚಕ, stmtsp};
            newp->user3(1);  // Do not split again
            lastp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(newp);
            lastp = newp;
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstIf* nodep) override {
        ಆದರೆ (!m_graphp || m_no_ವಿಭಜಿಸು_Why) ಹಿಂತಿರುಗಿ;
        {
            VL_RESTORER(m_currIfp);
            m_currIfp = nodep;
            iterate_And_ಮುಂದಿನ_Null(nodep->condp());
        }
        ಪರಿಶೀಲನೆ_ಖಂಡ(nodep->thensp());
        ಪರಿಶೀಲನೆ_ಖಂಡ(nodep->elsesp());
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override {
        ಆದರೆ (!m_graphp || m_no_ವಿಭಜಿಸು_Why) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_ಒಳ_Dly);
        m_ಒಳ_Dly = ಸುಳ್ಳು;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Dly* nodep) override {
        ಆದರೆ (!m_graphp || m_no_ವಿಭಜಿಸು_Why) ಹಿಂತಿರುಗಿ;
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
        ಆದರೆ (m_graphp) m_no_ವಿಭಜಿಸು_Why = "Handle";
        ಭೇಟಿ(static_cast<Ast_ಗಂಟು*>(nodep));
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstJumpGo*) override {
        ಆದರೆ (!m_graphp || m_no_ವಿಭಜಿಸು_Why) ಹಿಂತಿರುಗಿ;
        m_no_ವಿಭಜಿಸು_Why = "JumpGo";
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        ಆದರೆ (!m_graphp || m_no_ವಿಭಜಿಸು_Why) ಹಿಂತಿರುಗಿ;
        UASSERT_OBJ(!m_stmtStackps.ಖಾಲಿಗಳು(), nodep, "Not under a statement");

        // Constant lookups can be ignored
        ಆದರೆ (nodep->varp()->isConst()) ಹಿಂತಿರುಗಿ;

        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = nodep->varScopep();

        // SPEEDUP: We add duplicate edges, that should be fixed
        ಆದರೆ (m_ಒಳ_Dly && nodep->access().isWriteOrRW()) {
            // Delayed variable: is different from non-delayed variable, writes to it
            // are not observable while executing this block (NBA not yet committed),
            // so add only a write edge to a separate 'post' vertex.
            ಆದರೆ (!vscp->user2p()) vscp->user2p(ಹೊಸ ವಿಭಜಿಸು_Var_Post_ಶೃಂಗ{m_graphp, vscp});
            ವಿಭಜಿಸು_Var_Post_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vpostp = vscp->user2u().ಗಮ್ಯ<ವಿಭಜಿಸು_Var_Post_ಶೃಂಗ*>();
            ಗಾಗಿ (ವಿಭಜಿಸು_Stmt_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_stmtStackps) ಕೂಡಿಸು_ಅಂಚು(vpostp, vtxp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->access().isWriteOrRW()) {
            // Regular (non-blocking) write: Need to maintain program-flow order
            ಆದರೆ (!vscp->user1p()) vscp->user1p(ಹೊಸ ವಿಭಜಿಸು_Var_Std_ಶೃಂಗ{m_graphp, vscp});
            ವಿಭಜಿಸು_Var_Std_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vstdp = vscp->user1u().ಗಮ್ಯ<ವಿಭಜಿಸು_Var_Std_ಶೃಂಗ*>();
            ಗಾಗಿ (ವಿಭಜಿಸು_Stmt_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_stmtStackps) ಕೂಡಿಸು_ಅಂಚು(vstdp, vtxp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Regular (non-blocking) read: Need to maintain program-flow order
            ಆದರೆ (!vscp->user1p()) vscp->user1p(ಹೊಸ ವಿಭಜಿಸು_Var_Std_ಶೃಂಗ{m_graphp, vscp});
            ವಿಭಜಿಸು_Var_Std_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vstdp = vscp->user1u().ಗಮ್ಯ<ವಿಭಜಿಸು_Var_Std_ಶೃಂಗ*>();
            ಗಾಗಿ (ವಿಭಜಿಸು_Stmt_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_stmtStackps) {
                // If this is an if statement it only depends on refs in its
                // own condition only (not those in its branches). For other
                // statements, just record the referene as normal.
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstIf* ಸ್ಥಿರಾಂಕ ifp = VN_CAST(vtxp->nodep(), If)) {
                    ಆದರೆ (ifp != m_currIfp) ಮುಂದುವರಿ;
                }
                ಕೂಡಿಸು_ಅಂಚು(vtxp, vstdp);
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        // Outside AstAlways, just descend
        ಆದರೆ (!m_graphp) {
            iterateChildren(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        // Early exit if decided not to split
        ಆದರೆ (m_no_ವಿಭಜಿಸು_Why) ಹಿಂತಿರುಗಿ;

        UASSERT_OBJ(!m_stmtStackps.ಖಾಲಿಗಳು(), nodep, "Not under a statement");

        // Timing control prevents splitting
        ಆದರೆ (nodep->isTimingControl()) {
            m_no_ವಿಭಜಿಸು_Why = "TimingControl";
            ಹಿಂತಿರುಗಿ;
        }

        // All impure statements must be grouped together.
        ಆದರೆ (!nodep->is_ಶುದ್ಧ()) {
            ಆದರೆ (!m_impureVtxp) m_impureVtxp = ಹೊಸ ವಿಭಜಿಸು_Impure_ಶೃಂಗ{m_graphp, nodep};
            // One edge is enough to find the weakly connected components, but
            // it must point at the impure vertex, so it is an out edge (input
            // dependency) of any enclosing 'if' to prevent pruning.
            ಗಾಗಿ (ವಿಭಜಿಸು_Stmt_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_stmtStackps) ಕೂಡಿಸು_ಅಂಚು(vtxp, m_impureVtxp);
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ವಿಭಜಿಸು_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~ವಿಭಜಿಸು_Visitor() override { V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Split always", m_statSplits); }
    VL_UNCOPYABLE(ವಿಭಜಿಸು_Visitor);

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(AstNetlist* nodep) { ವಿಭಜಿಸು_Visitor{nodep}; }
};

//######################################################################
// Split class functions

ಶೂನ್ಯ V3_ವಿಭಜಿಸು::ವಿಭಜಿಸು_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    ವಿಭಜಿಸು_Visitor::apply(nodep);
    V3Global::dump_Check_Global_ವೃಕ್ಷ("split", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
