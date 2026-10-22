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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Split.h"

#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <string>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Support classes

വർഗ്ഗം Split_കണ്ണി_Vertex VL_NOT_FINAL : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(Split_കണ്ണി_Vertex, V3GraphVertex)
    Ast_കണ്ണി* സ്ഥിരം m_nodep;

സംരക്ഷിത:
    Split_കണ്ണി_Vertex(V3Graph* graphp, Ast_കണ്ണി* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    // ACCESSORS
    std::string name() സ്ഥിരം override {
        തിരികെ cvtToHex(m_nodep) + ' ' + m_nodep->prettyTypeName();
    }

പൊതു:
    Ast_കണ്ണി* nodep() സ്ഥിരം { തിരികെ m_nodep; }
};

വർഗ്ഗം SplitImpureVertex final : പൊതു Split_കണ്ണി_Vertex {
    VL_RTTI_IMPL(SplitImpureVertex, Split_കണ്ണി_Vertex)

    std::string name() സ്ഥിരം override { തിരികെ "*IMPURE*"; }
    std::string dotColor() സ്ഥിരം override { തിരികെ "green"; }

പൊതു:
    explicit SplitImpureVertex(V3Graph* graphp, Ast_കണ്ണി* nodep)
        : Split_കണ്ണി_Vertex{graphp, nodep} {}
};

വർഗ്ഗം SplitStmtVertex final : പൊതു Split_കണ്ണി_Vertex {
    VL_RTTI_IMPL(SplitStmtVertex, Split_കണ്ണി_Vertex)

    std::string dotColor() സ്ഥിരം override { തിരികെ "yellow"; }

പൊതു:
    SplitStmtVertex(V3Graph* graphp, Ast_കണ്ണി* nodep)
        : Split_കണ്ണി_Vertex{graphp, nodep} {}
};

വർഗ്ഗം SplitVarStdVertex final : പൊതു Split_കണ്ണി_Vertex {
    VL_RTTI_IMPL(SplitVarStdVertex, Split_കണ്ണി_Vertex)

    std::string dotColor() സ്ഥിരം override { തിരികെ "skyblue"; }

പൊതു:
    SplitVarStdVertex(V3Graph* graphp, AstVarScope* vscp)
        : Split_കണ്ണി_Vertex{graphp, vscp} {}
};

വർഗ്ഗം SplitVarPostVertex final : പൊതു Split_കണ്ണി_Vertex {
    VL_RTTI_IMPL(SplitVarPostVertex, Split_കണ്ണി_Vertex)

    std::string name() സ്ഥിരം override { തിരികെ "POST "s + Split_കണ്ണി_Vertex::name(); }
    std::string dotColor() സ്ഥിരം override { തിരികെ "CadetBlue"; }

പൊതു:
    SplitVarPostVertex(V3Graph* graphp, AstVarScope* vscp)
        : Split_കണ്ണി_Vertex{graphp, vscp} {}
};

വർഗ്ഗം SplitVisitor final : പൊതു VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> SplitVarStdVertex*: Regular program-flow variable vertex
    // AstVarScope::user2p  -> SplitVarPostVertex*: NBA written delayed variable vertex
    // Ast{StmtIsh}::user1p -> SplitStmtVertex*

    // NODE STATE
    // AstAlways::user3     -> bool: Block created by splitting, needs no further splitting
    സ്ഥിരം VNUser3InUse m_inuser3;

    // STATE
    V3Graph* m_graphp = nullptr;  // Dependency graph to analyze statement connectivity
    std::vector<SplitStmtVertex*> m_stmtStackps;  // Current statements being tracked
    SplitImpureVertex* m_impureVtxp = nullptr;  // Vertex connecting impure statements
    സ്ഥിരം അക്ഷരം* m_noSplitWhy = nullptr;  // Reason current block cannot be split
    തർക്കം m_inDly = അസത്യം;  // Inside AstAssignDly Lhs
    സ്ഥിരം AstIf* m_currIfp = nullptr;  // The AstIf whose condition is currently visited
    VDouble0 m_statSplits;  // Statistic tracking

    // METHODS
    ശൂന്യം addEdge(V3GraphVertex* fromp, V3GraphVertex* മുകൾ) {
        പുതിയ V3GraphEdge{m_graphp, fromp, മുകൾ, 1};
    }

    // Iterate the given list of statements, building the dependency graph
    ശൂന്യം scanBlock(Ast_കണ്ണി* stmtsp) {
        എങ്കിൽ (m_noSplitWhy) തിരികെ;
        വേണ്ടി (Ast_കണ്ണി* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Skip comments. They have no dependencies at all, so would always
            // form an independent component, and hence a split block, of their
            // own, which would be subsequently deleted as it does nothing.
            എങ്കിൽ (VN_IS(stmtp, Comment)) തുടരുക;
            UASSERT_OBJ(!stmtp->user1p(), stmtp, "user1p should not be set");
            SplitStmtVertex* സ്ഥിരം vtxp = പുതിയ SplitStmtVertex{m_graphp, stmtp};
            stmtp->user1p(vtxp);
            m_stmtStackps.തള്ളുക_back(vtxp);
            iterate(stmtp);
            m_stmtStackps.എടുക്കുക_back();
        }
    }

    // Remove unnecessary edges, then color to find weakly connected components
    uint32_t colorAlwaysGraph() {
        എങ്കിൽ (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_built", അസത്യം);

        // Prune duplicate edges. Not necessary for correctness, but simplifies dumps.
        m_graphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        എങ്കിൽ (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nodup", അസത്യം);

        // Remove variable vertices that are not written in the block.
        // These are input-only to the block, so carry no dependency.
        വേണ്ടി (V3GraphVertex* സ്ഥിരം vtxp : m_graphp->vertices().unlinkable()) {
            SplitVarStdVertex* സ്ഥിരം vstdp = vtxp->cast<SplitVarStdVertex>();
            എങ്കിൽ (!vstdp || !vstdp->outEmpty()) തുടരുക;
            UINFOTREE(9, vstdp->nodep(), "", "Will remove deps on block input var:");
            vstdp->nodep()->user1p(nullptr);  // Don't leave a dangling pointer behind
            VL_DO_DANGLING(vstdp->unlinkDelete(m_graphp), vstdp);
        }
        എങ്കിൽ (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_noinputs", അസത്യം);

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
        വേണ്ടി (V3GraphVertex* സ്ഥിരം vtxp : m_graphp->vertices().unlinkable()) {
            SplitStmtVertex* സ്ഥിരം stmtVtxp = vtxp->cast<SplitStmtVertex>();
            എങ്കിൽ (!stmtVtxp || !VN_IS(stmtVtxp->nodep(), If)) തുടരുക;
            // Can't remove if dependent on a variable written in the block
            എങ്കിൽ (!stmtVtxp->outEmpty()) തുടരുക;
            // Depends only on block inputs, so can be split. Remove the vertex.
            stmtVtxp->nodep()->user1p(nullptr);
            stmtVtxp->unlinkDelete(m_graphp);
        }
        എങ്കിൽ (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nofreeifs", അസത്യം);

        // Weak coloring to determine what must stay together in a single block
        സ്ഥിരം uint32_t അക്കം_Colors = m_graphp->weaklyConnected(&V3GraphEdge::followAlwaysTrue);
        എങ്കിൽ (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_colored", അസത്യം);
        തിരികെ അക്കം_Colors;
    }

    // Take the statements of the given list, and return them distributed into one list per color.
    സ്ഥിതം std::vector<Ast_കണ്ണി*> splitStatements(Ast_കണ്ണി* stmtsp, uint32_t അക്കം_Colors) {
        std::vector<Ast_കണ്ണി*> result{അക്കം_Colors, nullptr};
        വേണ്ടി (Ast_കണ്ണി *stmtp = stmtsp, *nextp = nullptr; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();  // 'stmtp' is unlinked below

            // Comments are dropped if the block is split
            എങ്കിൽ (VN_IS(stmtp, Comment)) തുടരുക;

            // Pick up the statement vertex, which might be nullptr after pruning during analysis
            സ്ഥിരം SplitStmtVertex* സ്ഥിരം vtxp = stmtp->user1u().to<SplitStmtVertex*>();

            // If statements are duplicated for each split branch
            എങ്കിൽ (AstIf* സ്ഥിരം ifp = VN_CAST(stmtp, If)) {
                സ്ഥിരം auto thens = splitStatements(ifp->thensp(), അക്കം_Colors);
                സ്ഥിരം auto elses = splitStatements(ifp->elsesp(), അക്കം_Colors);
                File_വരി* സ്ഥിരം flp = ifp->fileline();
                // Rebuild the 'if' for each color present in either branch
                തർക്കം empty = സത്യം;
                വേണ്ടി (uint32_t color = 0; color < അക്കം_Colors; ++color) {
                    എങ്കിൽ (!thens[color] && !elses[color]) തുടരുക;
                    empty = അസത്യം;
                    // The condition is cloned for each color. An impure condition
                    // keeps the 'if' and all it holds in one component, so just once.
                    AstIf* സ്ഥിരം clonep = പുതിയ AstIf{flp, ifp->condp()->clone_വൃക്ഷം(സത്യം),
                                                    thens[color], elses[color]};
                    // Preserve pragmas from unique if's so assertions work properly
                    clonep->uniquePragma(ifp->uniquePragma());
                    clonep->unique0Pragma(ifp->unique0Pragma());
                    clonep->priorityPragma(ifp->priorityPragma());
                    result[color] = Ast_കണ്ണി::addNext(result[color], clonep);
                }
                // There is nothing under the 'if' to guard. If its vertex was
                // removed as having no dependencies at all, then its
                // condition reads only block inputs and is pure, so the whole
                // 'if' can go. Otherwise the condition might have a side
                // effect, so keep just the condition, evaluated as a
                // statement, under the color of the 'if' itself.
                എങ്കിൽ (empty && vtxp) {
                    സ്ഥിരം uint32_t color = vtxp->color();
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = ifp->condp();
                    condp->unlinkFrBack();
                    result[color] = Ast_കണ്ണി::addNext(result[color], പുതിയ Ast_Stmt_വ്യഞ്ജകം{flp, condp});
                }
                തുടരുക;
            }

            // Move the leaf into its color's list
            സ്ഥിരം uint32_t color = vtxp->color();
            result[color] = Ast_കണ്ണി::addNext(result[color], stmtp->unlinkFrBack());
        }
        തിരികെ result;
    }

    // VISITORS
    ശൂന്യം visit(AstAlways* nodep) override {
        // Skip blocks created below
        എങ്കിൽ (nodep->user3()) തിരികെ;

        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_noSplitWhy);
        VL_RESTORER(m_inDly);
        V3Graph graph;
        m_graphp = &graph;
        m_impureVtxp = nullptr;
        m_noSplitWhy = nullptr;
        m_inDly = അസത്യം;
        UASSERT_OBJ(m_stmtStackps.empty(), nodep, "Statement stack not empty");

        // Build the graph
        സ്ഥിരം VNUser1InUse user1InUse;
        സ്ഥിരം VNUser2InUse user2InUse;
        scanBlock(nodep->stmtsp());

        // We might have to give up
        എങ്കിൽ (m_noSplitWhy) {
            UINFO(9, "  NoSplitBlock because " << m_noSplitWhy);
            തിരികെ;
        }

        // Color the graph to identify separable statements
        സ്ഥിരം uint32_t അക്കം_Colors = colorAlwaysGraph();
        // If the whole block is one component (or empty), then nothing to split
        എങ്കിൽ (അക്കം_Colors <= 1) തിരികെ;

        UINFO(6, "  splitting always " << nodep);

        // Count the number of new blocks inserted into the Ast: '1 -> n' split, so 'n - 1' extra
        m_statSplits += അക്കം_Colors - 1;

        // Unpick the statements out of the original block, into one list per color
        സ്ഥിരം auto lists = splitStatements(nodep->stmtsp(), അക്കം_Colors);
        UASSERT_OBJ(lists.size() == അക്കം_Colors, nodep, "Inconsistent split");

        // Whatever 'splitStatements' did not take, (comments, empty ifs) is not needed any more
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം restp = nodep->stmtsp()) {
            VL_DO_DANGLING(restp->unlinkFrBackWithNext()->delete_വൃക്ഷം(), restp);
        }

        // Every color has a statement in it. Reuse the original block for the
        // first color, and add a new block after it for each of the rest.
        UASSERT_OBJ(lists.front(), nodep, "Color with no statements");
        nodep->addStmtsp(lists.front());
        Ast_കണ്ണി* lastp = nodep;
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        സ്ഥിരം VAlwaysKwd kwd = nodep->keyword();
        വേണ്ടി (size_t i = 1; i < അക്കം_Colors; ++i) {
            Ast_കണ്ണി* സ്ഥിരം stmtsp = lists[i];
            UASSERT_OBJ(stmtsp, nodep, "Color with no statements");
            AstAlways* സ്ഥിരം newp = പുതിയ AstAlways{flp, kwd, nullptr, stmtsp};
            newp->user3(1);  // Do not split again
            lastp->addNextHere(newp);
            lastp = newp;
        }
    }

    ശൂന്യം visit(AstIf* nodep) override {
        എങ്കിൽ (!m_graphp || m_noSplitWhy) തിരികെ;
        {
            VL_RESTORER(m_currIfp);
            m_currIfp = nodep;
            iterateAndNextNull(nodep->condp());
        }
        scanBlock(nodep->thensp());
        scanBlock(nodep->elsesp());
    }

    ശൂന്യം visit(Ast_വ്യഞ്ജകം_Stmt* nodep) override {
        എങ്കിൽ (!m_graphp || m_noSplitWhy) തിരികെ;
        VL_RESTORER(m_inDly);
        m_inDly = അസത്യം;
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstAssignDly* nodep) override {
        എങ്കിൽ (!m_graphp || m_noSplitWhy) തിരികെ;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_inDly);
        m_inDly = സത്യം;
        iterate(nodep->lhsp());
    }

    ശൂന്യം visit(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        എങ്കിൽ (m_graphp) m_noSplitWhy = "Handle";
        visit(static_cast<Ast_കണ്ണി*>(nodep));
    }

    ശൂന്യം visit(AstJumpGo*) override {
        എങ്കിൽ (!m_graphp || m_noSplitWhy) തിരികെ;
        m_noSplitWhy = "JumpGo";
    }

    ശൂന്യം visit(AstVarRef* nodep) override {
        എങ്കിൽ (!m_graphp || m_noSplitWhy) തിരികെ;
        UASSERT_OBJ(!m_stmtStackps.empty(), nodep, "Not under a statement");

        // Constant lookups can be ignored
        എങ്കിൽ (nodep->varp()->isConst()) തിരികെ;

        AstVarScope* സ്ഥിരം vscp = nodep->varScopep();

        // SPEEDUP: We add duplicate edges, that should be fixed
        എങ്കിൽ (m_inDly && nodep->access().isWriteOrRW()) {
            // Delayed variable: is different from non-delayed variable, writes to it
            // are not observable while executing this block (NBA not yet committed),
            // so add only a write edge to a separate 'post' vertex.
            എങ്കിൽ (!vscp->user2p()) vscp->user2p(പുതിയ SplitVarPostVertex{m_graphp, vscp});
            SplitVarPostVertex* സ്ഥിരം vpostp = vscp->user2u().to<SplitVarPostVertex*>();
            വേണ്ടി (SplitStmtVertex* സ്ഥിരം vtxp : m_stmtStackps) addEdge(vpostp, vtxp);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->access().isWriteOrRW()) {
            // Regular (non-blocking) write: Need to maintain program-flow order
            എങ്കിൽ (!vscp->user1p()) vscp->user1p(പുതിയ SplitVarStdVertex{m_graphp, vscp});
            SplitVarStdVertex* സ്ഥിരം vstdp = vscp->user1u().to<SplitVarStdVertex*>();
            വേണ്ടി (SplitStmtVertex* സ്ഥിരം vtxp : m_stmtStackps) addEdge(vstdp, vtxp);
        } അല്ലെങ്കിൽ {
            // Regular (non-blocking) read: Need to maintain program-flow order
            എങ്കിൽ (!vscp->user1p()) vscp->user1p(പുതിയ SplitVarStdVertex{m_graphp, vscp});
            SplitVarStdVertex* സ്ഥിരം vstdp = vscp->user1u().to<SplitVarStdVertex*>();
            വേണ്ടി (SplitStmtVertex* സ്ഥിരം vtxp : m_stmtStackps) {
                // If this is an if statement it only depends on refs in its
                // own condition only (not those in its branches). For other
                // statements, just record the referene as normal.
                എങ്കിൽ (സ്ഥിരം AstIf* സ്ഥിരം ifp = VN_CAST(vtxp->nodep(), If)) {
                    എങ്കിൽ (ifp != m_currIfp) തുടരുക;
                }
                addEdge(vtxp, vstdp);
            }
        }
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        // Outside AstAlways, just descend
        എങ്കിൽ (!m_graphp) {
            iterateChildren(nodep);
            തിരികെ;
        }
        // Early exit if decided not to split
        എങ്കിൽ (m_noSplitWhy) തിരികെ;

        UASSERT_OBJ(!m_stmtStackps.empty(), nodep, "Not under a statement");

        // Timing control prevents splitting
        എങ്കിൽ (nodep->isTimingControl()) {
            m_noSplitWhy = "TimingControl";
            തിരികെ;
        }

        // All impure statements must be grouped together.
        എങ്കിൽ (!nodep->isPure()) {
            എങ്കിൽ (!m_impureVtxp) m_impureVtxp = പുതിയ SplitImpureVertex{m_graphp, nodep};
            // One edge is enough to find the weakly connected components, but
            // it must point at the impure vertex, so it is an out edge (input
            // dependency) of any enclosing 'if' to prevent pruning.
            വേണ്ടി (SplitStmtVertex* സ്ഥിരം vtxp : m_stmtStackps) addEdge(vtxp, m_impureVtxp);
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
    explicit SplitVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SplitVisitor() override { V3Stats::addStat("Optimizations, Split always", m_statSplits); }
    VL_UNCOPYABLE(SplitVisitor);

പൊതു:
    സ്ഥിതം ശൂന്യം apply(AstNetlist* nodep) { SplitVisitor{nodep}; }
};

//######################################################################
// Split class functions

ശൂന്യം V3Split::splitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    SplitVisitor::apply(nodep);
    V3Global::dump_Check_Global_വൃക്ഷം("split", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
