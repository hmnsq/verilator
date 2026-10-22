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

ವರ್ಗ Split_ಗಂಟು_Vertex VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(Split_ಗಂಟು_Vertex, V3GraphVertex)
    Ast_ಗಂಟು* ಧ್ರುವ m_nodep;

ರಕ್ಷಿತ:
    Split_ಗಂಟು_Vertex(V3Graph* graphp, Ast_ಗಂಟು* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    // ACCESSORS
    std::string name() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ cvtToHex(m_nodep) + ' ' + m_nodep->prettyTypeName();
    }

ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು* nodep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_nodep; }
};

ವರ್ಗ SplitImpureVertex final : ಸಾರ್ವಜನಿಕ Split_ಗಂಟು_Vertex {
    VL_RTTI_IMPL(SplitImpureVertex, Split_ಗಂಟು_Vertex)

    std::string name() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "*IMPURE*"; }
    std::string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "green"; }

ಸಾರ್ವಜನಿಕ:
    explicit SplitImpureVertex(V3Graph* graphp, Ast_ಗಂಟು* nodep)
        : Split_ಗಂಟು_Vertex{graphp, nodep} {}
};

ವರ್ಗ SplitStmtVertex final : ಸಾರ್ವಜನಿಕ Split_ಗಂಟು_Vertex {
    VL_RTTI_IMPL(SplitStmtVertex, Split_ಗಂಟು_Vertex)

    std::string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "yellow"; }

ಸಾರ್ವಜನಿಕ:
    SplitStmtVertex(V3Graph* graphp, Ast_ಗಂಟು* nodep)
        : Split_ಗಂಟು_Vertex{graphp, nodep} {}
};

ವರ್ಗ SplitVarStdVertex final : ಸಾರ್ವಜನಿಕ Split_ಗಂಟು_Vertex {
    VL_RTTI_IMPL(SplitVarStdVertex, Split_ಗಂಟು_Vertex)

    std::string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "skyblue"; }

ಸಾರ್ವಜನಿಕ:
    SplitVarStdVertex(V3Graph* graphp, AstVarScope* vscp)
        : Split_ಗಂಟು_Vertex{graphp, vscp} {}
};

ವರ್ಗ SplitVarPostVertex final : ಸಾರ್ವಜನಿಕ Split_ಗಂಟು_Vertex {
    VL_RTTI_IMPL(SplitVarPostVertex, Split_ಗಂಟು_Vertex)

    std::string name() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "POST "s + Split_ಗಂಟು_Vertex::name(); }
    std::string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "CadetBlue"; }

ಸಾರ್ವಜನಿಕ:
    SplitVarPostVertex(V3Graph* graphp, AstVarScope* vscp)
        : Split_ಗಂಟು_Vertex{graphp, vscp} {}
};

ವರ್ಗ SplitVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> SplitVarStdVertex*: Regular program-flow variable vertex
    // AstVarScope::user2p  -> SplitVarPostVertex*: NBA written delayed variable vertex
    // Ast{StmtIsh}::user1p -> SplitStmtVertex*

    // NODE STATE
    // AstAlways::user3     -> bool: Block created by splitting, needs no further splitting
    ಧ್ರುವ VNUser3InUse m_inuser3;

    // STATE
    V3Graph* m_graphp = nullptr;  // Dependency graph to analyze statement connectivity
    std::vector<SplitStmtVertex*> m_stmtStackps;  // Current statements being tracked
    SplitImpureVertex* m_impureVtxp = nullptr;  // Vertex connecting impure statements
    ಧ್ರುವ ವರ್ಣ* m_noSplitWhy = nullptr;  // Reason current block cannot be split
    ತಾರ್ಕಿಕ m_inDly = ಅಸತ್ಯ;  // Inside AstAssignDly Lhs
    ಧ್ರುವ AstIf* m_currIfp = nullptr;  // The AstIf whose condition is currently visited
    VDouble0 m_statSplits;  // Statistic tracking

    // METHODS
    ಶೂನ್ಯ addEdge(V3GraphVertex* fromp, V3GraphVertex* ತುದಿ) {
        ನೂತನ V3GraphEdge{m_graphp, fromp, ತುದಿ, 1};
    }

    // Iterate the given list of statements, building the dependency graph
    ಶೂನ್ಯ scanBlock(Ast_ಗಂಟು* stmtsp) {
        ಯದಿ (m_noSplitWhy) ಪ್ರತ್ಯಾಗಮ;
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Skip comments. They have no dependencies at all, so would always
            // form an independent component, and hence a split block, of their
            // own, which would be subsequently deleted as it does nothing.
            ಯದಿ (VN_IS(stmtp, Comment)) ಅನುವರ್ತ;
            UASSERT_OBJ(!stmtp->user1p(), stmtp, "user1p should not be set");
            SplitStmtVertex* ಧ್ರುವ vtxp = ನೂತನ SplitStmtVertex{m_graphp, stmtp};
            stmtp->user1p(vtxp);
            m_stmtStackps.ತಳ್ಳು_back(vtxp);
            iterate(stmtp);
            m_stmtStackps.ತೆಗೆ_back();
        }
    }

    // Remove unnecessary edges, then color to find weakly connected components
    uint32_t colorAlwaysGraph() {
        ಯದಿ (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_built", ಅಸತ್ಯ);

        // Prune duplicate edges. Not necessary for correctness, but simplifies dumps.
        m_graphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        ಯದಿ (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nodup", ಅಸತ್ಯ);

        // Remove variable vertices that are not written in the block.
        // These are input-only to the block, so carry no dependency.
        ಪ್ರತಿ (V3GraphVertex* ಧ್ರುವ vtxp : m_graphp->vertices().unlinkable()) {
            SplitVarStdVertex* ಧ್ರುವ vstdp = vtxp->cast<SplitVarStdVertex>();
            ಯದಿ (!vstdp || !vstdp->outEmpty()) ಅನುವರ್ತ;
            UINFOTREE(9, vstdp->nodep(), "", "Will remove deps on block input var:");
            vstdp->nodep()->user1p(nullptr);  // Don't leave a dangling pointer behind
            VL_DO_DANGLING(vstdp->unlinkDelete(m_graphp), vstdp);
        }
        ಯದಿ (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_noinputs", ಅಸತ್ಯ);

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
        ಪ್ರತಿ (V3GraphVertex* ಧ್ರುವ vtxp : m_graphp->vertices().unlinkable()) {
            SplitStmtVertex* ಧ್ರುವ stmtVtxp = vtxp->cast<SplitStmtVertex>();
            ಯದಿ (!stmtVtxp || !VN_IS(stmtVtxp->nodep(), If)) ಅನುವರ್ತ;
            // Can't remove if dependent on a variable written in the block
            ಯದಿ (!stmtVtxp->outEmpty()) ಅನುವರ್ತ;
            // Depends only on block inputs, so can be split. Remove the vertex.
            stmtVtxp->nodep()->user1p(nullptr);
            stmtVtxp->unlinkDelete(m_graphp);
        }
        ಯದಿ (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nofreeifs", ಅಸತ್ಯ);

        // Weak coloring to determine what must stay together in a single block
        ಧ್ರುವ uint32_t ಅಂಕೆ_Colors = m_graphp->weaklyConnected(&V3GraphEdge::followAlwaysTrue);
        ಯದಿ (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_colored", ಅಸತ್ಯ);
        ಪ್ರತ್ಯಾಗಮ ಅಂಕೆ_Colors;
    }

    // Take the statements of the given list, and return them distributed into one list per color.
    ಸ್ಥಿರ std::vector<Ast_ಗಂಟು*> splitStatements(Ast_ಗಂಟು* stmtsp, uint32_t ಅಂಕೆ_Colors) {
        std::vector<Ast_ಗಂಟು*> result{ಅಂಕೆ_Colors, nullptr};
        ಪ್ರತಿ (Ast_ಗಂಟು *stmtp = stmtsp, *nextp = nullptr; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();  // 'stmtp' is unlinked below

            // Comments are dropped if the block is split
            ಯದಿ (VN_IS(stmtp, Comment)) ಅನುವರ್ತ;

            // Pick up the statement vertex, which might be nullptr after pruning during analysis
            ಧ್ರುವ SplitStmtVertex* ಧ್ರುವ vtxp = stmtp->user1u().to<SplitStmtVertex*>();

            // If statements are duplicated for each split branch
            ಯದಿ (AstIf* ಧ್ರುವ ifp = VN_CAST(stmtp, If)) {
                ಧ್ರುವ auto thens = splitStatements(ifp->thensp(), ಅಂಕೆ_Colors);
                ಧ್ರುವ auto elses = splitStatements(ifp->elsesp(), ಅಂಕೆ_Colors);
                File_ಸಾಲು* ಧ್ರುವ flp = ifp->fileline();
                // Rebuild the 'if' for each color present in either branch
                ತಾರ್ಕಿಕ empty = ಸತ್ಯ;
                ಪ್ರತಿ (uint32_t color = 0; color < ಅಂಕೆ_Colors; ++color) {
                    ಯದಿ (!thens[color] && !elses[color]) ಅನುವರ್ತ;
                    empty = ಅಸತ್ಯ;
                    // The condition is cloned for each color. An impure condition
                    // keeps the 'if' and all it holds in one component, so just once.
                    AstIf* ಧ್ರುವ clonep = ನೂತನ AstIf{flp, ifp->condp()->clone_ವೃಕ್ಷ(ಸತ್ಯ),
                                                    thens[color], elses[color]};
                    // Preserve pragmas from unique if's so assertions work properly
                    clonep->uniquePragma(ifp->uniquePragma());
                    clonep->unique0Pragma(ifp->unique0Pragma());
                    clonep->priorityPragma(ifp->priorityPragma());
                    result[color] = Ast_ಗಂಟು::addNext(result[color], clonep);
                }
                // There is nothing under the 'if' to guard. If its vertex was
                // removed as having no dependencies at all, then its
                // condition reads only block inputs and is pure, so the whole
                // 'if' can go. Otherwise the condition might have a side
                // effect, so keep just the condition, evaluated as a
                // statement, under the color of the 'if' itself.
                ಯದಿ (empty && vtxp) {
                    ಧ್ರುವ uint32_t color = vtxp->color();
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = ifp->condp();
                    condp->unlinkFrBack();
                    result[color] = Ast_ಗಂಟು::addNext(result[color], ನೂತನ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{flp, condp});
                }
                ಅನುವರ್ತ;
            }

            // Move the leaf into its color's list
            ಧ್ರುವ uint32_t color = vtxp->color();
            result[color] = Ast_ಗಂಟು::addNext(result[color], stmtp->unlinkFrBack());
        }
        ಪ್ರತ್ಯಾಗಮ result;
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstAlways* nodep) override {
        // Skip blocks created below
        ಯದಿ (nodep->user3()) ಪ್ರತ್ಯಾಗಮ;

        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_noSplitWhy);
        VL_RESTORER(m_inDly);
        V3Graph graph;
        m_graphp = &graph;
        m_impureVtxp = nullptr;
        m_noSplitWhy = nullptr;
        m_inDly = ಅಸತ್ಯ;
        UASSERT_OBJ(m_stmtStackps.empty(), nodep, "Statement stack not empty");

        // Build the graph
        ಧ್ರುವ VNUser1InUse user1InUse;
        ಧ್ರುವ VNUser2InUse user2InUse;
        scanBlock(nodep->stmtsp());

        // We might have to give up
        ಯದಿ (m_noSplitWhy) {
            UINFO(9, "  NoSplitBlock because " << m_noSplitWhy);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Color the graph to identify separable statements
        ಧ್ರುವ uint32_t ಅಂಕೆ_Colors = colorAlwaysGraph();
        // If the whole block is one component (or empty), then nothing to split
        ಯದಿ (ಅಂಕೆ_Colors <= 1) ಪ್ರತ್ಯಾಗಮ;

        UINFO(6, "  splitting always " << nodep);

        // Count the number of new blocks inserted into the Ast: '1 -> n' split, so 'n - 1' extra
        m_statSplits += ಅಂಕೆ_Colors - 1;

        // Unpick the statements out of the original block, into one list per color
        ಧ್ರುವ auto lists = splitStatements(nodep->stmtsp(), ಅಂಕೆ_Colors);
        UASSERT_OBJ(lists.size() == ಅಂಕೆ_Colors, nodep, "Inconsistent split");

        // Whatever 'splitStatements' did not take, (comments, empty ifs) is not needed any more
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ restp = nodep->stmtsp()) {
            VL_DO_DANGLING(restp->unlinkFrBackWithNext()->delete_ವೃಕ್ಷ(), restp);
        }

        // Every color has a statement in it. Reuse the original block for the
        // first color, and add a new block after it for each of the rest.
        UASSERT_OBJ(lists.front(), nodep, "Color with no statements");
        nodep->addStmtsp(lists.front());
        Ast_ಗಂಟು* lastp = nodep;
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        ಧ್ರುವ VAlwaysKwd kwd = nodep->keyword();
        ಪ್ರತಿ (size_t i = 1; i < ಅಂಕೆ_Colors; ++i) {
            Ast_ಗಂಟು* ಧ್ರುವ stmtsp = lists[i];
            UASSERT_OBJ(stmtsp, nodep, "Color with no statements");
            AstAlways* ಧ್ರುವ newp = ನೂತನ AstAlways{flp, kwd, nullptr, stmtsp};
            newp->user3(1);  // Do not split again
            lastp->addNextHere(newp);
            lastp = newp;
        }
    }

    ಶೂನ್ಯ visit(AstIf* nodep) override {
        ಯದಿ (!m_graphp || m_noSplitWhy) ಪ್ರತ್ಯಾಗಮ;
        {
            VL_RESTORER(m_currIfp);
            m_currIfp = nodep;
            iterateAndNextNull(nodep->condp());
        }
        scanBlock(nodep->thensp());
        scanBlock(nodep->elsesp());
    }

    ಶೂನ್ಯ visit(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override {
        ಯದಿ (!m_graphp || m_noSplitWhy) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_inDly);
        m_inDly = ಅಸತ್ಯ;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstAssignDly* nodep) override {
        ಯದಿ (!m_graphp || m_noSplitWhy) ಪ್ರತ್ಯಾಗಮ;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_inDly);
        m_inDly = ಸತ್ಯ;
        iterate(nodep->lhsp());
    }

    ಶೂನ್ಯ visit(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        ಯದಿ (m_graphp) m_noSplitWhy = "Handle";
        visit(static_cast<Ast_ಗಂಟು*>(nodep));
    }

    ಶೂನ್ಯ visit(AstJumpGo*) override {
        ಯದಿ (!m_graphp || m_noSplitWhy) ಪ್ರತ್ಯಾಗಮ;
        m_noSplitWhy = "JumpGo";
    }

    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        ಯದಿ (!m_graphp || m_noSplitWhy) ಪ್ರತ್ಯಾಗಮ;
        UASSERT_OBJ(!m_stmtStackps.empty(), nodep, "Not under a statement");

        // Constant lookups can be ignored
        ಯದಿ (nodep->varp()->isConst()) ಪ್ರತ್ಯಾಗಮ;

        AstVarScope* ಧ್ರುವ vscp = nodep->varScopep();

        // SPEEDUP: We add duplicate edges, that should be fixed
        ಯದಿ (m_inDly && nodep->access().isWriteOrRW()) {
            // Delayed variable: is different from non-delayed variable, writes to it
            // are not observable while executing this block (NBA not yet committed),
            // so add only a write edge to a separate 'post' vertex.
            ಯದಿ (!vscp->user2p()) vscp->user2p(ನೂತನ SplitVarPostVertex{m_graphp, vscp});
            SplitVarPostVertex* ಧ್ರುವ vpostp = vscp->user2u().to<SplitVarPostVertex*>();
            ಪ್ರತಿ (SplitStmtVertex* ಧ್ರುವ vtxp : m_stmtStackps) addEdge(vpostp, vtxp);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->access().isWriteOrRW()) {
            // Regular (non-blocking) write: Need to maintain program-flow order
            ಯದಿ (!vscp->user1p()) vscp->user1p(ನೂತನ SplitVarStdVertex{m_graphp, vscp});
            SplitVarStdVertex* ಧ್ರುವ vstdp = vscp->user1u().to<SplitVarStdVertex*>();
            ಪ್ರತಿ (SplitStmtVertex* ಧ್ರುವ vtxp : m_stmtStackps) addEdge(vstdp, vtxp);
        } ಅನ್ಯಥಾ {
            // Regular (non-blocking) read: Need to maintain program-flow order
            ಯದಿ (!vscp->user1p()) vscp->user1p(ನೂತನ SplitVarStdVertex{m_graphp, vscp});
            SplitVarStdVertex* ಧ್ರುವ vstdp = vscp->user1u().to<SplitVarStdVertex*>();
            ಪ್ರತಿ (SplitStmtVertex* ಧ್ರುವ vtxp : m_stmtStackps) {
                // If this is an if statement it only depends on refs in its
                // own condition only (not those in its branches). For other
                // statements, just record the referene as normal.
                ಯದಿ (ಧ್ರುವ AstIf* ಧ್ರುವ ifp = VN_CAST(vtxp->nodep(), If)) {
                    ಯದಿ (ifp != m_currIfp) ಅನುವರ್ತ;
                }
                addEdge(vtxp, vstdp);
            }
        }
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        // Outside AstAlways, just descend
        ಯದಿ (!m_graphp) {
            iterateChildren(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Early exit if decided not to split
        ಯದಿ (m_noSplitWhy) ಪ್ರತ್ಯಾಗಮ;

        UASSERT_OBJ(!m_stmtStackps.empty(), nodep, "Not under a statement");

        // Timing control prevents splitting
        ಯದಿ (nodep->isTimingControl()) {
            m_noSplitWhy = "TimingControl";
            ಪ್ರತ್ಯಾಗಮ;
        }

        // All impure statements must be grouped together.
        ಯದಿ (!nodep->isPure()) {
            ಯದಿ (!m_impureVtxp) m_impureVtxp = ನೂತನ SplitImpureVertex{m_graphp, nodep};
            // One edge is enough to find the weakly connected components, but
            // it must point at the impure vertex, so it is an out edge (input
            // dependency) of any enclosing 'if' to prevent pruning.
            ಪ್ರತಿ (SplitStmtVertex* ಧ್ರುವ vtxp : m_stmtStackps) addEdge(vtxp, m_impureVtxp);
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
    explicit SplitVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SplitVisitor() override { V3Stats::addStat("Optimizations, Split always", m_statSplits); }
    VL_UNCOPYABLE(SplitVisitor);

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(AstNetlist* nodep) { SplitVisitor{nodep}; }
};

//######################################################################
// Split class functions

ಶೂನ್ಯ V3Split::splitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    SplitVisitor::apply(nodep);
    V3Global::dump_Check_Global_ವೃಕ್ಷ("split", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
