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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Split.h"

#অন্তর্ভাৱ "V3Graph.h"
#অন্তর্ভাৱ "V3Stats.h"

#অন্তর্ভাৱ <string>
#অন্তর্ভাৱ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Support classes

শ্ৰেণী Split_গ্ৰন্থি_Vertex VL_NOT_FINAL : সার্ৱজনিক V3GraphVertex {
    VL_RTTI_IMPL(Split_গ্ৰন্থি_Vertex, V3GraphVertex)
    Ast_গ্ৰন্থি* ধ্রুৱ m_nodep;

রক্ষিত:
    Split_গ্ৰন্থি_Vertex(V3Graph* graphp, Ast_গ্ৰন্থি* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    // ACCESSORS
    std::string name() ধ্রুৱ override {
        ঘূৰাই cvtToHex(m_nodep) + ' ' + m_nodep->prettyTypeName();
    }

সার্ৱজনিক:
    Ast_গ্ৰন্থি* nodep() ধ্রুৱ { ঘূৰাই m_nodep; }
};

শ্ৰেণী SplitImpureVertex final : সার্ৱজনিক Split_গ্ৰন্থি_Vertex {
    VL_RTTI_IMPL(SplitImpureVertex, Split_গ্ৰন্থি_Vertex)

    std::string name() ধ্রুৱ override { ঘূৰাই "*IMPURE*"; }
    std::string dotColor() ধ্রুৱ override { ঘূৰাই "green"; }

সার্ৱজনিক:
    explicit SplitImpureVertex(V3Graph* graphp, Ast_গ্ৰন্থি* nodep)
        : Split_গ্ৰন্থি_Vertex{graphp, nodep} {}
};

শ্ৰেণী SplitStmtVertex final : সার্ৱজনিক Split_গ্ৰন্থি_Vertex {
    VL_RTTI_IMPL(SplitStmtVertex, Split_গ্ৰন্থি_Vertex)

    std::string dotColor() ধ্রুৱ override { ঘূৰাই "yellow"; }

সার্ৱজনিক:
    SplitStmtVertex(V3Graph* graphp, Ast_গ্ৰন্থি* nodep)
        : Split_গ্ৰন্থি_Vertex{graphp, nodep} {}
};

শ্ৰেণী SplitVarStdVertex final : সার্ৱজনিক Split_গ্ৰন্থি_Vertex {
    VL_RTTI_IMPL(SplitVarStdVertex, Split_গ্ৰন্থি_Vertex)

    std::string dotColor() ধ্রুৱ override { ঘূৰাই "skyblue"; }

সার্ৱজনিক:
    SplitVarStdVertex(V3Graph* graphp, AstVarScope* vscp)
        : Split_গ্ৰন্থি_Vertex{graphp, vscp} {}
};

শ্ৰেণী SplitVarPostVertex final : সার্ৱজনিক Split_গ্ৰন্থি_Vertex {
    VL_RTTI_IMPL(SplitVarPostVertex, Split_গ্ৰন্থি_Vertex)

    std::string name() ধ্রুৱ override { ঘূৰাই "POST "s + Split_গ্ৰন্থি_Vertex::name(); }
    std::string dotColor() ধ্রুৱ override { ঘূৰাই "CadetBlue"; }

সার্ৱজনিক:
    SplitVarPostVertex(V3Graph* graphp, AstVarScope* vscp)
        : Split_গ্ৰন্থি_Vertex{graphp, vscp} {}
};

শ্ৰেণী SplitVisitor final : সার্ৱজনিক VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> SplitVarStdVertex*: Regular program-flow variable vertex
    // AstVarScope::user2p  -> SplitVarPostVertex*: NBA written delayed variable vertex
    // Ast{StmtIsh}::user1p -> SplitStmtVertex*

    // NODE STATE
    // AstAlways::user3     -> bool: Block created by splitting, needs no further splitting
    ধ্রুৱ VNUser3InUse m_inuser3;

    // STATE
    V3Graph* m_graphp = nullptr;  // Dependency graph to analyze statement connectivity
    std::vector<SplitStmtVertex*> m_stmtStackps;  // Current statements being tracked
    SplitImpureVertex* m_impureVtxp = nullptr;  // Vertex connecting impure statements
    ধ্রুৱ ৱর্ণ* m_noSplitWhy = nullptr;  // Reason current block cannot be split
    তার্কিক m_inDly = মিছা;  // Inside AstAssignDly Lhs
    ধ্রুৱ AstIf* m_currIfp = nullptr;  // The AstIf whose condition is currently visited
    VDouble0 m_statSplits;  // Statistic tracking

    // METHODS
    শূন্য addEdge(V3GraphVertex* fromp, V3GraphVertex* শীৰ্ষ) {
        নূতন V3GraphEdge{m_graphp, fromp, শীৰ্ষ, 1};
    }

    // Iterate the given list of statements, building the dependency graph
    শূন্য scanBlock(Ast_গ্ৰন্থি* stmtsp) {
        যদি (m_noSplitWhy) ঘূৰাই;
        বাবে (Ast_গ্ৰন্থি* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Skip comments. They have no dependencies at all, so would always
            // form an independent component, and hence a split block, of their
            // own, which would be subsequently deleted as it does nothing.
            যদি (VN_IS(stmtp, Comment)) অনুৱর্ত;
            UASSERT_OBJ(!stmtp->user1p(), stmtp, "user1p should not be set");
            SplitStmtVertex* ধ্রুৱ vtxp = নূতন SplitStmtVertex{m_graphp, stmtp};
            stmtp->user1p(vtxp);
            m_stmtStackps.ঠেলো_back(vtxp);
            iterate(stmtp);
            m_stmtStackps.বেৰ_কৰো_back();
        }
    }

    // Remove unnecessary edges, then color to find weakly connected components
    uint32_t colorAlwaysGraph() {
        যদি (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_built", মিছা);

        // Prune duplicate edges. Not necessary for correctness, but simplifies dumps.
        m_graphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        যদি (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nodup", মিছা);

        // Remove variable vertices that are not written in the block.
        // These are input-only to the block, so carry no dependency.
        বাবে (V3GraphVertex* ধ্রুৱ vtxp : m_graphp->vertices().unlinkable()) {
            SplitVarStdVertex* ধ্রুৱ vstdp = vtxp->cast<SplitVarStdVertex>();
            যদি (!vstdp || !vstdp->outEmpty()) অনুৱর্ত;
            UINFOTREE(9, vstdp->nodep(), "", "Will remove deps on block input var:");
            vstdp->nodep()->user1p(nullptr);  // Don't leave a dangling pointer behind
            VL_DO_DANGLING(vstdp->unlinkDelete(m_graphp), vstdp);
        }
        যদি (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_noinputs", মিছা);

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
        বাবে (V3GraphVertex* ধ্রুৱ vtxp : m_graphp->vertices().unlinkable()) {
            SplitStmtVertex* ধ্রুৱ stmtVtxp = vtxp->cast<SplitStmtVertex>();
            যদি (!stmtVtxp || !VN_IS(stmtVtxp->nodep(), If)) অনুৱর্ত;
            // Can't remove if dependent on a variable written in the block
            যদি (!stmtVtxp->outEmpty()) অনুৱর্ত;
            // Depends only on block inputs, so can be split. Remove the vertex.
            stmtVtxp->nodep()->user1p(nullptr);
            stmtVtxp->unlinkDelete(m_graphp);
        }
        যদি (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nofreeifs", মিছা);

        // Weak coloring to determine what must stay together in a single block
        ধ্রুৱ uint32_t অঙ্ক_Colors = m_graphp->weaklyConnected(&V3GraphEdge::followAlwaysTrue);
        যদি (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_colored", মিছা);
        ঘূৰাই অঙ্ক_Colors;
    }

    // Take the statements of the given list, and return them distributed into one list per color.
    স্থির std::vector<Ast_গ্ৰন্থি*> splitStatements(Ast_গ্ৰন্থি* stmtsp, uint32_t অঙ্ক_Colors) {
        std::vector<Ast_গ্ৰন্থি*> result{অঙ্ক_Colors, nullptr};
        বাবে (Ast_গ্ৰন্থি *stmtp = stmtsp, *nextp = nullptr; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();  // 'stmtp' is unlinked below

            // Comments are dropped if the block is split
            যদি (VN_IS(stmtp, Comment)) অনুৱর্ত;

            // Pick up the statement vertex, which might be nullptr after pruning during analysis
            ধ্রুৱ SplitStmtVertex* ধ্রুৱ vtxp = stmtp->user1u().to<SplitStmtVertex*>();

            // If statements are duplicated for each split branch
            যদি (AstIf* ধ্রুৱ ifp = VN_CAST(stmtp, If)) {
                ধ্রুৱ auto thens = splitStatements(ifp->thensp(), অঙ্ক_Colors);
                ধ্রুৱ auto elses = splitStatements(ifp->elsesp(), অঙ্ক_Colors);
                File_সাৰি* ধ্রুৱ flp = ifp->fileline();
                // Rebuild the 'if' for each color present in either branch
                তার্কিক empty = সঁচা;
                বাবে (uint32_t color = 0; color < অঙ্ক_Colors; ++color) {
                    যদি (!thens[color] && !elses[color]) অনুৱর্ত;
                    empty = মিছা;
                    // The condition is cloned for each color. An impure condition
                    // keeps the 'if' and all it holds in one component, so just once.
                    AstIf* ধ্রুৱ clonep = নূতন AstIf{flp, ifp->condp()->clone_বৃক্ষ(সঁচা),
                                                    thens[color], elses[color]};
                    // Preserve pragmas from unique if's so assertions work properly
                    clonep->uniquePragma(ifp->uniquePragma());
                    clonep->unique0Pragma(ifp->unique0Pragma());
                    clonep->priorityPragma(ifp->priorityPragma());
                    result[color] = Ast_গ্ৰন্থি::addNext(result[color], clonep);
                }
                // There is nothing under the 'if' to guard. If its vertex was
                // removed as having no dependencies at all, then its
                // condition reads only block inputs and is pure, so the whole
                // 'if' can go. Otherwise the condition might have a side
                // effect, so keep just the condition, evaluated as a
                // statement, under the color of the 'if' itself.
                যদি (empty && vtxp) {
                    ধ্রুৱ uint32_t color = vtxp->color();
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ condp = ifp->condp();
                    condp->unlinkFrBack();
                    result[color] = Ast_গ্ৰন্থি::addNext(result[color], নূতন Ast_Stmt_ৰাশিমালা{flp, condp});
                }
                অনুৱর্ত;
            }

            // Move the leaf into its color's list
            ধ্রুৱ uint32_t color = vtxp->color();
            result[color] = Ast_গ্ৰন্থি::addNext(result[color], stmtp->unlinkFrBack());
        }
        ঘূৰাই result;
    }

    // VISITORS
    শূন্য visit(AstAlways* nodep) override {
        // Skip blocks created below
        যদি (nodep->user3()) ঘূৰাই;

        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_noSplitWhy);
        VL_RESTORER(m_inDly);
        V3Graph graph;
        m_graphp = &graph;
        m_impureVtxp = nullptr;
        m_noSplitWhy = nullptr;
        m_inDly = মিছা;
        UASSERT_OBJ(m_stmtStackps.empty(), nodep, "Statement stack not empty");

        // Build the graph
        ধ্রুৱ VNUser1InUse user1InUse;
        ধ্রুৱ VNUser2InUse user2InUse;
        scanBlock(nodep->stmtsp());

        // We might have to give up
        যদি (m_noSplitWhy) {
            UINFO(9, "  NoSplitBlock because " << m_noSplitWhy);
            ঘূৰাই;
        }

        // Color the graph to identify separable statements
        ধ্রুৱ uint32_t অঙ্ক_Colors = colorAlwaysGraph();
        // If the whole block is one component (or empty), then nothing to split
        যদি (অঙ্ক_Colors <= 1) ঘূৰাই;

        UINFO(6, "  splitting always " << nodep);

        // Count the number of new blocks inserted into the Ast: '1 -> n' split, so 'n - 1' extra
        m_statSplits += অঙ্ক_Colors - 1;

        // Unpick the statements out of the original block, into one list per color
        ধ্রুৱ auto lists = splitStatements(nodep->stmtsp(), অঙ্ক_Colors);
        UASSERT_OBJ(lists.size() == অঙ্ক_Colors, nodep, "Inconsistent split");

        // Whatever 'splitStatements' did not take, (comments, empty ifs) is not needed any more
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ restp = nodep->stmtsp()) {
            VL_DO_DANGLING(restp->unlinkFrBackWithNext()->delete_বৃক্ষ(), restp);
        }

        // Every color has a statement in it. Reuse the original block for the
        // first color, and add a new block after it for each of the rest.
        UASSERT_OBJ(lists.front(), nodep, "Color with no statements");
        nodep->addStmtsp(lists.front());
        Ast_গ্ৰন্থি* lastp = nodep;
        File_সাৰি* ধ্রুৱ flp = nodep->fileline();
        ধ্রুৱ VAlwaysKwd kwd = nodep->keyword();
        বাবে (size_t i = 1; i < অঙ্ক_Colors; ++i) {
            Ast_গ্ৰন্থি* ধ্রুৱ stmtsp = lists[i];
            UASSERT_OBJ(stmtsp, nodep, "Color with no statements");
            AstAlways* ধ্রুৱ newp = নূতন AstAlways{flp, kwd, nullptr, stmtsp};
            newp->user3(1);  // Do not split again
            lastp->addNextHere(newp);
            lastp = newp;
        }
    }

    শূন্য visit(AstIf* nodep) override {
        যদি (!m_graphp || m_noSplitWhy) ঘূৰাই;
        {
            VL_RESTORER(m_currIfp);
            m_currIfp = nodep;
            iterateAndNextNull(nodep->condp());
        }
        scanBlock(nodep->thensp());
        scanBlock(nodep->elsesp());
    }

    শূন্য visit(Ast_ৰাশিমালা_Stmt* nodep) override {
        যদি (!m_graphp || m_noSplitWhy) ঘূৰাই;
        VL_RESTORER(m_inDly);
        m_inDly = মিছা;
        iterateChildren(nodep);
    }

    শূন্য visit(AstAssignDly* nodep) override {
        যদি (!m_graphp || m_noSplitWhy) ঘূৰাই;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_inDly);
        m_inDly = সঁচা;
        iterate(nodep->lhsp());
    }

    শূন্য visit(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        যদি (m_graphp) m_noSplitWhy = "Handle";
        visit(static_cast<Ast_গ্ৰন্থি*>(nodep));
    }

    শূন্য visit(AstJumpGo*) override {
        যদি (!m_graphp || m_noSplitWhy) ঘূৰাই;
        m_noSplitWhy = "JumpGo";
    }

    শূন্য visit(AstVarRef* nodep) override {
        যদি (!m_graphp || m_noSplitWhy) ঘূৰাই;
        UASSERT_OBJ(!m_stmtStackps.empty(), nodep, "Not under a statement");

        // Constant lookups can be ignored
        যদি (nodep->varp()->isConst()) ঘূৰাই;

        AstVarScope* ধ্রুৱ vscp = nodep->varScopep();

        // SPEEDUP: We add duplicate edges, that should be fixed
        যদি (m_inDly && nodep->access().isWriteOrRW()) {
            // Delayed variable: is different from non-delayed variable, writes to it
            // are not observable while executing this block (NBA not yet committed),
            // so add only a write edge to a separate 'post' vertex.
            যদি (!vscp->user2p()) vscp->user2p(নূতন SplitVarPostVertex{m_graphp, vscp});
            SplitVarPostVertex* ধ্রুৱ vpostp = vscp->user2u().to<SplitVarPostVertex*>();
            বাবে (SplitStmtVertex* ধ্রুৱ vtxp : m_stmtStackps) addEdge(vpostp, vtxp);
        } অন্যথা যদি (nodep->access().isWriteOrRW()) {
            // Regular (non-blocking) write: Need to maintain program-flow order
            যদি (!vscp->user1p()) vscp->user1p(নূতন SplitVarStdVertex{m_graphp, vscp});
            SplitVarStdVertex* ধ্রুৱ vstdp = vscp->user1u().to<SplitVarStdVertex*>();
            বাবে (SplitStmtVertex* ধ্রুৱ vtxp : m_stmtStackps) addEdge(vstdp, vtxp);
        } অন্যথা {
            // Regular (non-blocking) read: Need to maintain program-flow order
            যদি (!vscp->user1p()) vscp->user1p(নূতন SplitVarStdVertex{m_graphp, vscp});
            SplitVarStdVertex* ধ্রুৱ vstdp = vscp->user1u().to<SplitVarStdVertex*>();
            বাবে (SplitStmtVertex* ধ্রুৱ vtxp : m_stmtStackps) {
                // If this is an if statement it only depends on refs in its
                // own condition only (not those in its branches). For other
                // statements, just record the referene as normal.
                যদি (ধ্রুৱ AstIf* ধ্রুৱ ifp = VN_CAST(vtxp->nodep(), If)) {
                    যদি (ifp != m_currIfp) অনুৱর্ত;
                }
                addEdge(vtxp, vstdp);
            }
        }
    }

    শূন্য visit(Ast_গ্ৰন্থি* nodep) override {
        // Outside AstAlways, just descend
        যদি (!m_graphp) {
            iterateChildren(nodep);
            ঘূৰাই;
        }
        // Early exit if decided not to split
        যদি (m_noSplitWhy) ঘূৰাই;

        UASSERT_OBJ(!m_stmtStackps.empty(), nodep, "Not under a statement");

        // Timing control prevents splitting
        যদি (nodep->isTimingControl()) {
            m_noSplitWhy = "TimingControl";
            ঘূৰাই;
        }

        // All impure statements must be grouped together.
        যদি (!nodep->isPure()) {
            যদি (!m_impureVtxp) m_impureVtxp = নূতন SplitImpureVertex{m_graphp, nodep};
            // One edge is enough to find the weakly connected components, but
            // it must point at the impure vertex, so it is an out edge (input
            // dependency) of any enclosing 'if' to prevent pruning.
            বাবে (SplitStmtVertex* ধ্রুৱ vtxp : m_stmtStackps) addEdge(vtxp, m_impureVtxp);
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
    explicit SplitVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SplitVisitor() override { V3Stats::addStat("Optimizations, Split always", m_statSplits); }
    VL_UNCOPYABLE(SplitVisitor);

সার্ৱজনিক:
    স্থির শূন্য apply(AstNetlist* nodep) { SplitVisitor{nodep}; }
};

//######################################################################
// Split class functions

শূন্য V3Split::splitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    SplitVisitor::apply(nodep);
    V3Global::dump_Check_Global_বৃক্ষ("split", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
