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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Split.h"

#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <string>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Support classes

ଶ୍ରେଣୀ Split_ଗଣ୍ଠି_Vertex VL_NOT_FINAL : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(Split_ଗଣ୍ଠି_Vertex, V3GraphVertex)
    Ast_ଗଣ୍ଠି* ଅଚଳ m_nodep;

ସୁରକ୍ଷିତ:
    Split_ଗଣ୍ଠି_Vertex(V3Graph* graphp, Ast_ଗଣ୍ଠି* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    // ACCESSORS
    std::string name() ଅଚଳ override {
        ଫେରନ୍ତୁ cvtToHex(m_nodep) + ' ' + m_nodep->prettyTypeName();
    }

ସର୍ବସାଧାରଣ:
    Ast_ଗଣ୍ଠି* nodep() ଅଚଳ { ଫେରନ୍ତୁ m_nodep; }
};

ଶ୍ରେଣୀ SplitImpureVertex final : ସର୍ବସାଧାରଣ Split_ଗଣ୍ଠି_Vertex {
    VL_RTTI_IMPL(SplitImpureVertex, Split_ଗଣ୍ଠି_Vertex)

    std::string name() ଅଚଳ override { ଫେରନ୍ତୁ "*IMPURE*"; }
    std::string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "green"; }

ସର୍ବସାଧାରଣ:
    explicit SplitImpureVertex(V3Graph* graphp, Ast_ଗଣ୍ଠି* nodep)
        : Split_ଗଣ୍ଠି_Vertex{graphp, nodep} {}
};

ଶ୍ରେଣୀ SplitStmtVertex final : ସର୍ବସାଧାରଣ Split_ଗଣ୍ଠି_Vertex {
    VL_RTTI_IMPL(SplitStmtVertex, Split_ଗଣ୍ଠି_Vertex)

    std::string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "yellow"; }

ସର୍ବସାଧାରଣ:
    SplitStmtVertex(V3Graph* graphp, Ast_ଗଣ୍ଠି* nodep)
        : Split_ଗଣ୍ଠି_Vertex{graphp, nodep} {}
};

ଶ୍ରେଣୀ SplitVarStdVertex final : ସର୍ବସାଧାରଣ Split_ଗଣ୍ଠି_Vertex {
    VL_RTTI_IMPL(SplitVarStdVertex, Split_ଗଣ୍ଠି_Vertex)

    std::string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "skyblue"; }

ସର୍ବସାଧାରଣ:
    SplitVarStdVertex(V3Graph* graphp, AstVarScope* vscp)
        : Split_ଗଣ୍ଠି_Vertex{graphp, vscp} {}
};

ଶ୍ରେଣୀ SplitVarPostVertex final : ସର୍ବସାଧାରଣ Split_ଗଣ୍ଠି_Vertex {
    VL_RTTI_IMPL(SplitVarPostVertex, Split_ଗଣ୍ଠି_Vertex)

    std::string name() ଅଚଳ override { ଫେରନ୍ତୁ "POST "s + Split_ଗଣ୍ଠି_Vertex::name(); }
    std::string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "CadetBlue"; }

ସର୍ବସାଧାରଣ:
    SplitVarPostVertex(V3Graph* graphp, AstVarScope* vscp)
        : Split_ଗଣ୍ଠି_Vertex{graphp, vscp} {}
};

ଶ୍ରେଣୀ SplitVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> SplitVarStdVertex*: Regular program-flow variable vertex
    // AstVarScope::user2p  -> SplitVarPostVertex*: NBA written delayed variable vertex
    // Ast{StmtIsh}::user1p -> SplitStmtVertex*

    // NODE STATE
    // AstAlways::user3     -> bool: Block created by splitting, needs no further splitting
    ଅଚଳ VNUser3InUse m_inuser3;

    // STATE
    V3Graph* m_graphp = nullptr;  // Dependency graph to analyze statement connectivity
    std::vector<SplitStmtVertex*> m_stmtStackps;  // Current statements being tracked
    SplitImpureVertex* m_impureVtxp = nullptr;  // Vertex connecting impure statements
    ଅଚଳ ଅକ୍ଷର* m_noSplitWhy = nullptr;  // Reason current block cannot be split
    ତାର୍କିକ m_inDly = ମିଥ୍ୟା;  // Inside AstAssignDly Lhs
    ଅଚଳ AstIf* m_currIfp = nullptr;  // The AstIf whose condition is currently visited
    VDouble0 m_statSplits;  // Statistic tracking

    // METHODS
    ଶୂନ୍ୟ addEdge(V3GraphVertex* fromp, V3GraphVertex* ଶୀର୍ଷ) {
        ନୂତନ V3GraphEdge{m_graphp, fromp, ଶୀର୍ଷ, 1};
    }

    // Iterate the given list of statements, building the dependency graph
    ଶୂନ୍ୟ scanBlock(Ast_ଗଣ୍ଠି* stmtsp) {
        ଯଦି (m_noSplitWhy) ଫେରନ୍ତୁ;
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Skip comments. They have no dependencies at all, so would always
            // form an independent component, and hence a split block, of their
            // own, which would be subsequently deleted as it does nothing.
            ଯଦି (VN_IS(stmtp, Comment)) ଜାରି;
            UASSERT_OBJ(!stmtp->user1p(), stmtp, "user1p should not be set");
            SplitStmtVertex* ଅଚଳ vtxp = ନୂତନ SplitStmtVertex{m_graphp, stmtp};
            stmtp->user1p(vtxp);
            m_stmtStackps.ଠେଲ_back(vtxp);
            iterate(stmtp);
            m_stmtStackps.କାଢ଼_back();
        }
    }

    // Remove unnecessary edges, then color to find weakly connected components
    uint32_t colorAlwaysGraph() {
        ଯଦି (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_built", ମିଥ୍ୟା);

        // Prune duplicate edges. Not necessary for correctness, but simplifies dumps.
        m_graphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        ଯଦି (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nodup", ମିଥ୍ୟା);

        // Remove variable vertices that are not written in the block.
        // These are input-only to the block, so carry no dependency.
        ପାଇଁ (V3GraphVertex* ଅଚଳ vtxp : m_graphp->vertices().unlinkable()) {
            SplitVarStdVertex* ଅଚଳ vstdp = vtxp->cast<SplitVarStdVertex>();
            ଯଦି (!vstdp || !vstdp->outEmpty()) ଜାରି;
            UINFOTREE(9, vstdp->nodep(), "", "Will remove deps on block input var:");
            vstdp->nodep()->user1p(nullptr);  // Don't leave a dangling pointer behind
            VL_DO_DANGLING(vstdp->unlinkDelete(m_graphp), vstdp);
        }
        ଯଦି (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_noinputs", ମିଥ୍ୟା);

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
        ପାଇଁ (V3GraphVertex* ଅଚଳ vtxp : m_graphp->vertices().unlinkable()) {
            SplitStmtVertex* ଅଚଳ stmtVtxp = vtxp->cast<SplitStmtVertex>();
            ଯଦି (!stmtVtxp || !VN_IS(stmtVtxp->nodep(), If)) ଜାରି;
            // Can't remove if dependent on a variable written in the block
            ଯଦି (!stmtVtxp->outEmpty()) ଜାରି;
            // Depends only on block inputs, so can be split. Remove the vertex.
            stmtVtxp->nodep()->user1p(nullptr);
            stmtVtxp->unlinkDelete(m_graphp);
        }
        ଯଦି (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nofreeifs", ମିଥ୍ୟା);

        // Weak coloring to determine what must stay together in a single block
        ଅଚଳ uint32_t ଅଙ୍କ_Colors = m_graphp->weaklyConnected(&V3GraphEdge::followAlwaysTrue);
        ଯଦି (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_colored", ମିଥ୍ୟା);
        ଫେରନ୍ତୁ ଅଙ୍କ_Colors;
    }

    // Take the statements of the given list, and return them distributed into one list per color.
    ସ୍ଥିର std::vector<Ast_ଗଣ୍ଠି*> splitStatements(Ast_ଗଣ୍ଠି* stmtsp, uint32_t ଅଙ୍କ_Colors) {
        std::vector<Ast_ଗଣ୍ଠି*> result{ଅଙ୍କ_Colors, nullptr};
        ପାଇଁ (Ast_ଗଣ୍ଠି *stmtp = stmtsp, *nextp = nullptr; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();  // 'stmtp' is unlinked below

            // Comments are dropped if the block is split
            ଯଦି (VN_IS(stmtp, Comment)) ଜାରି;

            // Pick up the statement vertex, which might be nullptr after pruning during analysis
            ଅଚଳ SplitStmtVertex* ଅଚଳ vtxp = stmtp->user1u().to<SplitStmtVertex*>();

            // If statements are duplicated for each split branch
            ଯଦି (AstIf* ଅଚଳ ifp = VN_CAST(stmtp, If)) {
                ଅଚଳ auto thens = splitStatements(ifp->thensp(), ଅଙ୍କ_Colors);
                ଅଚଳ auto elses = splitStatements(ifp->elsesp(), ଅଙ୍କ_Colors);
                File_ଧାଡ଼ି* ଅଚଳ flp = ifp->fileline();
                // Rebuild the 'if' for each color present in either branch
                ତାର୍କିକ empty = ସତ୍ୟ;
                ପାଇଁ (uint32_t color = 0; color < ଅଙ୍କ_Colors; ++color) {
                    ଯଦି (!thens[color] && !elses[color]) ଜାରି;
                    empty = ମିଥ୍ୟା;
                    // The condition is cloned for each color. An impure condition
                    // keeps the 'if' and all it holds in one component, so just once.
                    AstIf* ଅଚଳ clonep = ନୂତନ AstIf{flp, ifp->condp()->clone_ବୃକ୍ଷ(ସତ୍ୟ),
                                                    thens[color], elses[color]};
                    // Preserve pragmas from unique if's so assertions work properly
                    clonep->uniquePragma(ifp->uniquePragma());
                    clonep->unique0Pragma(ifp->unique0Pragma());
                    clonep->priorityPragma(ifp->priorityPragma());
                    result[color] = Ast_ଗଣ୍ଠି::addNext(result[color], clonep);
                }
                // There is nothing under the 'if' to guard. If its vertex was
                // removed as having no dependencies at all, then its
                // condition reads only block inputs and is pure, so the whole
                // 'if' can go. Otherwise the condition might have a side
                // effect, so keep just the condition, evaluated as a
                // statement, under the color of the 'if' itself.
                ଯଦି (empty && vtxp) {
                    ଅଚଳ uint32_t color = vtxp->color();
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = ifp->condp();
                    condp->unlinkFrBack();
                    result[color] = Ast_ଗଣ୍ଠି::addNext(result[color], ନୂତନ Ast_Stmt_ପଦସମଷ୍ଟି{flp, condp});
                }
                ଜାରି;
            }

            // Move the leaf into its color's list
            ଅଚଳ uint32_t color = vtxp->color();
            result[color] = Ast_ଗଣ୍ଠି::addNext(result[color], stmtp->unlinkFrBack());
        }
        ଫେରନ୍ତୁ result;
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstAlways* nodep) override {
        // Skip blocks created below
        ଯଦି (nodep->user3()) ଫେରନ୍ତୁ;

        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_noSplitWhy);
        VL_RESTORER(m_inDly);
        V3Graph graph;
        m_graphp = &graph;
        m_impureVtxp = nullptr;
        m_noSplitWhy = nullptr;
        m_inDly = ମିଥ୍ୟା;
        UASSERT_OBJ(m_stmtStackps.empty(), nodep, "Statement stack not empty");

        // Build the graph
        ଅଚଳ VNUser1InUse user1InUse;
        ଅଚଳ VNUser2InUse user2InUse;
        scanBlock(nodep->stmtsp());

        // We might have to give up
        ଯଦି (m_noSplitWhy) {
            UINFO(9, "  NoSplitBlock because " << m_noSplitWhy);
            ଫେରନ୍ତୁ;
        }

        // Color the graph to identify separable statements
        ଅଚଳ uint32_t ଅଙ୍କ_Colors = colorAlwaysGraph();
        // If the whole block is one component (or empty), then nothing to split
        ଯଦି (ଅଙ୍କ_Colors <= 1) ଫେରନ୍ତୁ;

        UINFO(6, "  splitting always " << nodep);

        // Count the number of new blocks inserted into the Ast: '1 -> n' split, so 'n - 1' extra
        m_statSplits += ଅଙ୍କ_Colors - 1;

        // Unpick the statements out of the original block, into one list per color
        ଅଚଳ auto lists = splitStatements(nodep->stmtsp(), ଅଙ୍କ_Colors);
        UASSERT_OBJ(lists.size() == ଅଙ୍କ_Colors, nodep, "Inconsistent split");

        // Whatever 'splitStatements' did not take, (comments, empty ifs) is not needed any more
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ restp = nodep->stmtsp()) {
            VL_DO_DANGLING(restp->unlinkFrBackWithNext()->delete_ବୃକ୍ଷ(), restp);
        }

        // Every color has a statement in it. Reuse the original block for the
        // first color, and add a new block after it for each of the rest.
        UASSERT_OBJ(lists.front(), nodep, "Color with no statements");
        nodep->addStmtsp(lists.front());
        Ast_ଗଣ୍ଠି* lastp = nodep;
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        ଅଚଳ VAlwaysKwd kwd = nodep->keyword();
        ପାଇଁ (size_t i = 1; i < ଅଙ୍କ_Colors; ++i) {
            Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = lists[i];
            UASSERT_OBJ(stmtsp, nodep, "Color with no statements");
            AstAlways* ଅଚଳ newp = ନୂତନ AstAlways{flp, kwd, nullptr, stmtsp};
            newp->user3(1);  // Do not split again
            lastp->addNextHere(newp);
            lastp = newp;
        }
    }

    ଶୂନ୍ୟ visit(AstIf* nodep) override {
        ଯଦି (!m_graphp || m_noSplitWhy) ଫେରନ୍ତୁ;
        {
            VL_RESTORER(m_currIfp);
            m_currIfp = nodep;
            iterateAndNextNull(nodep->condp());
        }
        scanBlock(nodep->thensp());
        scanBlock(nodep->elsesp());
    }

    ଶୂନ୍ୟ visit(Ast_ପଦସମଷ୍ଟି_Stmt* nodep) override {
        ଯଦି (!m_graphp || m_noSplitWhy) ଫେରନ୍ତୁ;
        VL_RESTORER(m_inDly);
        m_inDly = ମିଥ୍ୟା;
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstAssignDly* nodep) override {
        ଯଦି (!m_graphp || m_noSplitWhy) ଫେରନ୍ତୁ;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_inDly);
        m_inDly = ସତ୍ୟ;
        iterate(nodep->lhsp());
    }

    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        ଯଦି (m_graphp) m_noSplitWhy = "Handle";
        visit(static_cast<Ast_ଗଣ୍ଠି*>(nodep));
    }

    ଶୂନ୍ୟ visit(AstJumpGo*) override {
        ଯଦି (!m_graphp || m_noSplitWhy) ଫେରନ୍ତୁ;
        m_noSplitWhy = "JumpGo";
    }

    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        ଯଦି (!m_graphp || m_noSplitWhy) ଫେରନ୍ତୁ;
        UASSERT_OBJ(!m_stmtStackps.empty(), nodep, "Not under a statement");

        // Constant lookups can be ignored
        ଯଦି (nodep->varp()->isConst()) ଫେରନ୍ତୁ;

        AstVarScope* ଅଚଳ vscp = nodep->varScopep();

        // SPEEDUP: We add duplicate edges, that should be fixed
        ଯଦି (m_inDly && nodep->access().isWriteOrRW()) {
            // Delayed variable: is different from non-delayed variable, writes to it
            // are not observable while executing this block (NBA not yet committed),
            // so add only a write edge to a separate 'post' vertex.
            ଯଦି (!vscp->user2p()) vscp->user2p(ନୂତନ SplitVarPostVertex{m_graphp, vscp});
            SplitVarPostVertex* ଅଚଳ vpostp = vscp->user2u().to<SplitVarPostVertex*>();
            ପାଇଁ (SplitStmtVertex* ଅଚଳ vtxp : m_stmtStackps) addEdge(vpostp, vtxp);
        } ଅନ୍ୟଥା ଯଦି (nodep->access().isWriteOrRW()) {
            // Regular (non-blocking) write: Need to maintain program-flow order
            ଯଦି (!vscp->user1p()) vscp->user1p(ନୂତନ SplitVarStdVertex{m_graphp, vscp});
            SplitVarStdVertex* ଅଚଳ vstdp = vscp->user1u().to<SplitVarStdVertex*>();
            ପାଇଁ (SplitStmtVertex* ଅଚଳ vtxp : m_stmtStackps) addEdge(vstdp, vtxp);
        } ଅନ୍ୟଥା {
            // Regular (non-blocking) read: Need to maintain program-flow order
            ଯଦି (!vscp->user1p()) vscp->user1p(ନୂତନ SplitVarStdVertex{m_graphp, vscp});
            SplitVarStdVertex* ଅଚଳ vstdp = vscp->user1u().to<SplitVarStdVertex*>();
            ପାଇଁ (SplitStmtVertex* ଅଚଳ vtxp : m_stmtStackps) {
                // If this is an if statement it only depends on refs in its
                // own condition only (not those in its branches). For other
                // statements, just record the referene as normal.
                ଯଦି (ଅଚଳ AstIf* ଅଚଳ ifp = VN_CAST(vtxp->nodep(), If)) {
                    ଯଦି (ifp != m_currIfp) ଜାରି;
                }
                addEdge(vtxp, vstdp);
            }
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        // Outside AstAlways, just descend
        ଯଦି (!m_graphp) {
            iterateChildren(nodep);
            ଫେରନ୍ତୁ;
        }
        // Early exit if decided not to split
        ଯଦି (m_noSplitWhy) ଫେରନ୍ତୁ;

        UASSERT_OBJ(!m_stmtStackps.empty(), nodep, "Not under a statement");

        // Timing control prevents splitting
        ଯଦି (nodep->isTimingControl()) {
            m_noSplitWhy = "TimingControl";
            ଫେରନ୍ତୁ;
        }

        // All impure statements must be grouped together.
        ଯଦି (!nodep->isPure()) {
            ଯଦି (!m_impureVtxp) m_impureVtxp = ନୂତନ SplitImpureVertex{m_graphp, nodep};
            // One edge is enough to find the weakly connected components, but
            // it must point at the impure vertex, so it is an out edge (input
            // dependency) of any enclosing 'if' to prevent pruning.
            ପାଇଁ (SplitStmtVertex* ଅଚଳ vtxp : m_stmtStackps) addEdge(vtxp, m_impureVtxp);
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
    explicit SplitVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SplitVisitor() override { V3Stats::addStat("Optimizations, Split always", m_statSplits); }
    VL_UNCOPYABLE(SplitVisitor);

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(AstNetlist* nodep) { SplitVisitor{nodep}; }
};

//######################################################################
// Split class functions

ଶୂନ୍ୟ V3Split::splitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    SplitVisitor::apply(nodep);
    V3Global::dump_Check_Global_ବୃକ୍ଷ("split", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
