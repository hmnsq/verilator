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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Split.h"

#અન્તર્ભાવ "V3Graph.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <string>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Support classes

વર્ગ Split_ગાંઠ_Vertex VL_NOT_FINAL : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(Split_ગાંઠ_Vertex, V3GraphVertex)
    Ast_ગાંઠ* અચલ m_nodep;

સુરક્ષિત:
    Split_ગાંઠ_Vertex(V3Graph* graphp, Ast_ગાંઠ* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    // ACCESSORS
    std::string name() અચલ override {
        પાછા cvtToHex(m_nodep) + ' ' + m_nodep->prettyTypeName();
    }

જાહેર:
    Ast_ગાંઠ* nodep() અચલ { પાછા m_nodep; }
};

વર્ગ SplitImpureVertex final : જાહેર Split_ગાંઠ_Vertex {
    VL_RTTI_IMPL(SplitImpureVertex, Split_ગાંઠ_Vertex)

    std::string name() અચલ override { પાછા "*IMPURE*"; }
    std::string dotColor() અચલ override { પાછા "green"; }

જાહેર:
    explicit SplitImpureVertex(V3Graph* graphp, Ast_ગાંઠ* nodep)
        : Split_ગાંઠ_Vertex{graphp, nodep} {}
};

વર્ગ SplitStmtVertex final : જાહેર Split_ગાંઠ_Vertex {
    VL_RTTI_IMPL(SplitStmtVertex, Split_ગાંઠ_Vertex)

    std::string dotColor() અચલ override { પાછા "yellow"; }

જાહેર:
    SplitStmtVertex(V3Graph* graphp, Ast_ગાંઠ* nodep)
        : Split_ગાંઠ_Vertex{graphp, nodep} {}
};

વર્ગ SplitVarStdVertex final : જાહેર Split_ગાંઠ_Vertex {
    VL_RTTI_IMPL(SplitVarStdVertex, Split_ગાંઠ_Vertex)

    std::string dotColor() અચલ override { પાછા "skyblue"; }

જાહેર:
    SplitVarStdVertex(V3Graph* graphp, AstVarScope* vscp)
        : Split_ગાંઠ_Vertex{graphp, vscp} {}
};

વર્ગ SplitVarPostVertex final : જાહેર Split_ગાંઠ_Vertex {
    VL_RTTI_IMPL(SplitVarPostVertex, Split_ગાંઠ_Vertex)

    std::string name() અચલ override { પાછા "POST "s + Split_ગાંઠ_Vertex::name(); }
    std::string dotColor() અચલ override { પાછા "CadetBlue"; }

જાહેર:
    SplitVarPostVertex(V3Graph* graphp, AstVarScope* vscp)
        : Split_ગાંઠ_Vertex{graphp, vscp} {}
};

વર્ગ SplitVisitor final : જાહેર VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> SplitVarStdVertex*: Regular program-flow variable vertex
    // AstVarScope::user2p  -> SplitVarPostVertex*: NBA written delayed variable vertex
    // Ast{StmtIsh}::user1p -> SplitStmtVertex*

    // NODE STATE
    // AstAlways::user3     -> bool: Block created by splitting, needs no further splitting
    અચલ VNUser3InUse m_inuser3;

    // STATE
    V3Graph* m_graphp = nullptr;  // Dependency graph to analyze statement connectivity
    std::vector<SplitStmtVertex*> m_stmtStackps;  // Current statements being tracked
    SplitImpureVertex* m_impureVtxp = nullptr;  // Vertex connecting impure statements
    અચલ અક્ષર* m_noSplitWhy = nullptr;  // Reason current block cannot be split
    તાર્કિક m_inDly = ખોટું;  // Inside AstAssignDly Lhs
    અચલ AstIf* m_currIfp = nullptr;  // The AstIf whose condition is currently visited
    VDouble0 m_statSplits;  // Statistic tracking

    // METHODS
    શૂન્ય addEdge(V3GraphVertex* fromp, V3GraphVertex* ટોચ) {
        નવું V3GraphEdge{m_graphp, fromp, ટોચ, 1};
    }

    // Iterate the given list of statements, building the dependency graph
    શૂન્ય scanBlock(Ast_ગાંઠ* stmtsp) {
        જો (m_noSplitWhy) પાછા;
        માટે (Ast_ગાંઠ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Skip comments. They have no dependencies at all, so would always
            // form an independent component, and hence a split block, of their
            // own, which would be subsequently deleted as it does nothing.
            જો (VN_IS(stmtp, Comment)) ચાલુ;
            UASSERT_OBJ(!stmtp->user1p(), stmtp, "user1p should not be set");
            SplitStmtVertex* અચલ vtxp = નવું SplitStmtVertex{m_graphp, stmtp};
            stmtp->user1p(vtxp);
            m_stmtStackps.ધકેલો_back(vtxp);
            iterate(stmtp);
            m_stmtStackps.કાઢો_back();
        }
    }

    // Remove unnecessary edges, then color to find weakly connected components
    uint32_t colorAlwaysGraph() {
        જો (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_built", ખોટું);

        // Prune duplicate edges. Not necessary for correctness, but simplifies dumps.
        m_graphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        જો (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nodup", ખોટું);

        // Remove variable vertices that are not written in the block.
        // These are input-only to the block, so carry no dependency.
        માટે (V3GraphVertex* અચલ vtxp : m_graphp->vertices().unlinkable()) {
            SplitVarStdVertex* અચલ vstdp = vtxp->cast<SplitVarStdVertex>();
            જો (!vstdp || !vstdp->outEmpty()) ચાલુ;
            UINFOTREE(9, vstdp->nodep(), "", "Will remove deps on block input var:");
            vstdp->nodep()->user1p(nullptr);  // Don't leave a dangling pointer behind
            VL_DO_DANGLING(vstdp->unlinkDelete(m_graphp), vstdp);
        }
        જો (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_noinputs", ખોટું);

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
        માટે (V3GraphVertex* અચલ vtxp : m_graphp->vertices().unlinkable()) {
            SplitStmtVertex* અચલ stmtVtxp = vtxp->cast<SplitStmtVertex>();
            જો (!stmtVtxp || !VN_IS(stmtVtxp->nodep(), If)) ચાલુ;
            // Can't remove if dependent on a variable written in the block
            જો (!stmtVtxp->outEmpty()) ચાલુ;
            // Depends only on block inputs, so can be split. Remove the vertex.
            stmtVtxp->nodep()->user1p(nullptr);
            stmtVtxp->unlinkDelete(m_graphp);
        }
        જો (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nofreeifs", ખોટું);

        // Weak coloring to determine what must stay together in a single block
        અચલ uint32_t અંક_Colors = m_graphp->weaklyConnected(&V3GraphEdge::followAlwaysTrue);
        જો (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_colored", ખોટું);
        પાછા અંક_Colors;
    }

    // Take the statements of the given list, and return them distributed into one list per color.
    સ્થિર std::vector<Ast_ગાંઠ*> splitStatements(Ast_ગાંઠ* stmtsp, uint32_t અંક_Colors) {
        std::vector<Ast_ગાંઠ*> result{અંક_Colors, nullptr};
        માટે (Ast_ગાંઠ *stmtp = stmtsp, *nextp = nullptr; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();  // 'stmtp' is unlinked below

            // Comments are dropped if the block is split
            જો (VN_IS(stmtp, Comment)) ચાલુ;

            // Pick up the statement vertex, which might be nullptr after pruning during analysis
            અચલ SplitStmtVertex* અચલ vtxp = stmtp->user1u().to<SplitStmtVertex*>();

            // If statements are duplicated for each split branch
            જો (AstIf* અચલ ifp = VN_CAST(stmtp, If)) {
                અચલ auto thens = splitStatements(ifp->thensp(), અંક_Colors);
                અચલ auto elses = splitStatements(ifp->elsesp(), અંક_Colors);
                File_લીટી* અચલ flp = ifp->fileline();
                // Rebuild the 'if' for each color present in either branch
                તાર્કિક empty = સાચું;
                માટે (uint32_t color = 0; color < અંક_Colors; ++color) {
                    જો (!thens[color] && !elses[color]) ચાલુ;
                    empty = ખોટું;
                    // The condition is cloned for each color. An impure condition
                    // keeps the 'if' and all it holds in one component, so just once.
                    AstIf* અચલ clonep = નવું AstIf{flp, ifp->condp()->clone_વૃક્ષ(સાચું),
                                                    thens[color], elses[color]};
                    // Preserve pragmas from unique if's so assertions work properly
                    clonep->uniquePragma(ifp->uniquePragma());
                    clonep->unique0Pragma(ifp->unique0Pragma());
                    clonep->priorityPragma(ifp->priorityPragma());
                    result[color] = Ast_ગાંઠ::addNext(result[color], clonep);
                }
                // There is nothing under the 'if' to guard. If its vertex was
                // removed as having no dependencies at all, then its
                // condition reads only block inputs and is pure, so the whole
                // 'if' can go. Otherwise the condition might have a side
                // effect, so keep just the condition, evaluated as a
                // statement, under the color of the 'if' itself.
                જો (empty && vtxp) {
                    અચલ uint32_t color = vtxp->color();
                    Ast_ગાંઠ_પદાવલિ* અચલ condp = ifp->condp();
                    condp->unlinkFrBack();
                    result[color] = Ast_ગાંઠ::addNext(result[color], નવું Ast_Stmt_પદાવલિ{flp, condp});
                }
                ચાલુ;
            }

            // Move the leaf into its color's list
            અચલ uint32_t color = vtxp->color();
            result[color] = Ast_ગાંઠ::addNext(result[color], stmtp->unlinkFrBack());
        }
        પાછા result;
    }

    // VISITORS
    શૂન્ય visit(AstAlways* nodep) override {
        // Skip blocks created below
        જો (nodep->user3()) પાછા;

        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_noSplitWhy);
        VL_RESTORER(m_inDly);
        V3Graph graph;
        m_graphp = &graph;
        m_impureVtxp = nullptr;
        m_noSplitWhy = nullptr;
        m_inDly = ખોટું;
        UASSERT_OBJ(m_stmtStackps.empty(), nodep, "Statement stack not empty");

        // Build the graph
        અચલ VNUser1InUse user1InUse;
        અચલ VNUser2InUse user2InUse;
        scanBlock(nodep->stmtsp());

        // We might have to give up
        જો (m_noSplitWhy) {
            UINFO(9, "  NoSplitBlock because " << m_noSplitWhy);
            પાછા;
        }

        // Color the graph to identify separable statements
        અચલ uint32_t અંક_Colors = colorAlwaysGraph();
        // If the whole block is one component (or empty), then nothing to split
        જો (અંક_Colors <= 1) પાછા;

        UINFO(6, "  splitting always " << nodep);

        // Count the number of new blocks inserted into the Ast: '1 -> n' split, so 'n - 1' extra
        m_statSplits += અંક_Colors - 1;

        // Unpick the statements out of the original block, into one list per color
        અચલ auto lists = splitStatements(nodep->stmtsp(), અંક_Colors);
        UASSERT_OBJ(lists.size() == અંક_Colors, nodep, "Inconsistent split");

        // Whatever 'splitStatements' did not take, (comments, empty ifs) is not needed any more
        જો (Ast_ગાંઠ* અચલ restp = nodep->stmtsp()) {
            VL_DO_DANGLING(restp->unlinkFrBackWithNext()->delete_વૃક્ષ(), restp);
        }

        // Every color has a statement in it. Reuse the original block for the
        // first color, and add a new block after it for each of the rest.
        UASSERT_OBJ(lists.front(), nodep, "Color with no statements");
        nodep->addStmtsp(lists.front());
        Ast_ગાંઠ* lastp = nodep;
        File_લીટી* અચલ flp = nodep->fileline();
        અચલ VAlwaysKwd kwd = nodep->keyword();
        માટે (size_t i = 1; i < અંક_Colors; ++i) {
            Ast_ગાંઠ* અચલ stmtsp = lists[i];
            UASSERT_OBJ(stmtsp, nodep, "Color with no statements");
            AstAlways* અચલ newp = નવું AstAlways{flp, kwd, nullptr, stmtsp};
            newp->user3(1);  // Do not split again
            lastp->addNextHere(newp);
            lastp = newp;
        }
    }

    શૂન્ય visit(AstIf* nodep) override {
        જો (!m_graphp || m_noSplitWhy) પાછા;
        {
            VL_RESTORER(m_currIfp);
            m_currIfp = nodep;
            iterateAndNextNull(nodep->condp());
        }
        scanBlock(nodep->thensp());
        scanBlock(nodep->elsesp());
    }

    શૂન્ય visit(Ast_પદાવલિ_Stmt* nodep) override {
        જો (!m_graphp || m_noSplitWhy) પાછા;
        VL_RESTORER(m_inDly);
        m_inDly = ખોટું;
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstAssignDly* nodep) override {
        જો (!m_graphp || m_noSplitWhy) પાછા;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        જો (Ast_ગાંઠ* અચલ controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_inDly);
        m_inDly = સાચું;
        iterate(nodep->lhsp());
    }

    શૂન્ય visit(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        જો (m_graphp) m_noSplitWhy = "Handle";
        visit(static_cast<Ast_ગાંઠ*>(nodep));
    }

    શૂન્ય visit(AstJumpGo*) override {
        જો (!m_graphp || m_noSplitWhy) પાછા;
        m_noSplitWhy = "JumpGo";
    }

    શૂન્ય visit(AstVarRef* nodep) override {
        જો (!m_graphp || m_noSplitWhy) પાછા;
        UASSERT_OBJ(!m_stmtStackps.empty(), nodep, "Not under a statement");

        // Constant lookups can be ignored
        જો (nodep->varp()->isConst()) પાછા;

        AstVarScope* અચલ vscp = nodep->varScopep();

        // SPEEDUP: We add duplicate edges, that should be fixed
        જો (m_inDly && nodep->access().isWriteOrRW()) {
            // Delayed variable: is different from non-delayed variable, writes to it
            // are not observable while executing this block (NBA not yet committed),
            // so add only a write edge to a separate 'post' vertex.
            જો (!vscp->user2p()) vscp->user2p(નવું SplitVarPostVertex{m_graphp, vscp});
            SplitVarPostVertex* અચલ vpostp = vscp->user2u().to<SplitVarPostVertex*>();
            માટે (SplitStmtVertex* અચલ vtxp : m_stmtStackps) addEdge(vpostp, vtxp);
        } નહીંતર જો (nodep->access().isWriteOrRW()) {
            // Regular (non-blocking) write: Need to maintain program-flow order
            જો (!vscp->user1p()) vscp->user1p(નવું SplitVarStdVertex{m_graphp, vscp});
            SplitVarStdVertex* અચલ vstdp = vscp->user1u().to<SplitVarStdVertex*>();
            માટે (SplitStmtVertex* અચલ vtxp : m_stmtStackps) addEdge(vstdp, vtxp);
        } નહીંતર {
            // Regular (non-blocking) read: Need to maintain program-flow order
            જો (!vscp->user1p()) vscp->user1p(નવું SplitVarStdVertex{m_graphp, vscp});
            SplitVarStdVertex* અચલ vstdp = vscp->user1u().to<SplitVarStdVertex*>();
            માટે (SplitStmtVertex* અચલ vtxp : m_stmtStackps) {
                // If this is an if statement it only depends on refs in its
                // own condition only (not those in its branches). For other
                // statements, just record the referene as normal.
                જો (અચલ AstIf* અચલ ifp = VN_CAST(vtxp->nodep(), If)) {
                    જો (ifp != m_currIfp) ચાલુ;
                }
                addEdge(vtxp, vstdp);
            }
        }
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        // Outside AstAlways, just descend
        જો (!m_graphp) {
            iterateChildren(nodep);
            પાછા;
        }
        // Early exit if decided not to split
        જો (m_noSplitWhy) પાછા;

        UASSERT_OBJ(!m_stmtStackps.empty(), nodep, "Not under a statement");

        // Timing control prevents splitting
        જો (nodep->isTimingControl()) {
            m_noSplitWhy = "TimingControl";
            પાછા;
        }

        // All impure statements must be grouped together.
        જો (!nodep->isPure()) {
            જો (!m_impureVtxp) m_impureVtxp = નવું SplitImpureVertex{m_graphp, nodep};
            // One edge is enough to find the weakly connected components, but
            // it must point at the impure vertex, so it is an out edge (input
            // dependency) of any enclosing 'if' to prevent pruning.
            માટે (SplitStmtVertex* અચલ vtxp : m_stmtStackps) addEdge(vtxp, m_impureVtxp);
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
    explicit SplitVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SplitVisitor() override { V3Stats::addStat("Optimizations, Split always", m_statSplits); }
    VL_UNCOPYABLE(SplitVisitor);

જાહેર:
    સ્થિર શૂન્ય apply(AstNetlist* nodep) { SplitVisitor{nodep}; }
};

//######################################################################
// Split class functions

શૂન્ય V3Split::splitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    SplitVisitor::apply(nodep);
    V3Global::dump_Check_Global_વૃક્ષ("split", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
