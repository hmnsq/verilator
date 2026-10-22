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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Split.h"

#समावेश "V3Graph.h"
#समावेश "V3Stats.h"

#समावेश <string>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Support classes

श्रेणी Split_गाँठ_Vertex VL_NOT_FINAL : खुला V3GraphVertex {
    VL_RTTI_IMPL(Split_गाँठ_Vertex, V3GraphVertex)
    Ast_गाँठ* स्थिर m_nodep;

रक्षित:
    Split_गाँठ_Vertex(V3Graph* graphp, Ast_गाँठ* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    // ACCESSORS
    std::string name() स्थिर override {
        वापस cvtToHex(m_nodep) + ' ' + m_nodep->prettyTypeName();
    }

खुला:
    Ast_गाँठ* nodep() स्थिर { वापस m_nodep; }
};

श्रेणी SplitImpureVertex final : खुला Split_गाँठ_Vertex {
    VL_RTTI_IMPL(SplitImpureVertex, Split_गाँठ_Vertex)

    std::string name() स्थिर override { वापस "*IMPURE*"; }
    std::string dotColor() स्थिर override { वापस "green"; }

खुला:
    explicit SplitImpureVertex(V3Graph* graphp, Ast_गाँठ* nodep)
        : Split_गाँठ_Vertex{graphp, nodep} {}
};

श्रेणी SplitStmtVertex final : खुला Split_गाँठ_Vertex {
    VL_RTTI_IMPL(SplitStmtVertex, Split_गाँठ_Vertex)

    std::string dotColor() स्थिर override { वापस "yellow"; }

खुला:
    SplitStmtVertex(V3Graph* graphp, Ast_गाँठ* nodep)
        : Split_गाँठ_Vertex{graphp, nodep} {}
};

श्रेणी SplitVarStdVertex final : खुला Split_गाँठ_Vertex {
    VL_RTTI_IMPL(SplitVarStdVertex, Split_गाँठ_Vertex)

    std::string dotColor() स्थिर override { वापस "skyblue"; }

खुला:
    SplitVarStdVertex(V3Graph* graphp, AstVarScope* vscp)
        : Split_गाँठ_Vertex{graphp, vscp} {}
};

श्रेणी SplitVarPostVertex final : खुला Split_गाँठ_Vertex {
    VL_RTTI_IMPL(SplitVarPostVertex, Split_गाँठ_Vertex)

    std::string name() स्थिर override { वापस "POST "s + Split_गाँठ_Vertex::name(); }
    std::string dotColor() स्थिर override { वापस "CadetBlue"; }

खुला:
    SplitVarPostVertex(V3Graph* graphp, AstVarScope* vscp)
        : Split_गाँठ_Vertex{graphp, vscp} {}
};

श्रेणी SplitVisitor final : खुला VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> SplitVarStdVertex*: Regular program-flow variable vertex
    // AstVarScope::user2p  -> SplitVarPostVertex*: NBA written delayed variable vertex
    // Ast{StmtIsh}::user1p -> SplitStmtVertex*

    // NODE STATE
    // AstAlways::user3     -> bool: Block created by splitting, needs no further splitting
    स्थिर VNUser3InUse m_inuser3;

    // STATE
    V3Graph* m_graphp = nullptr;  // Dependency graph to analyze statement connectivity
    std::vector<SplitStmtVertex*> m_stmtStackps;  // Current statements being tracked
    SplitImpureVertex* m_impureVtxp = nullptr;  // Vertex connecting impure statements
    स्थिर अक्षर* m_noSplitWhy = nullptr;  // Reason current block cannot be split
    bool m_inDly = false;  // Inside AstAssignDly Lhs
    स्थिर AstIf* m_currIfp = nullptr;  // The AstIf whose condition is currently visited
    VDouble0 m_statSplits;  // Statistic tracking

    // METHODS
    व्योम addEdge(V3GraphVertex* fromp, V3GraphVertex* शीर्ष) {
        नया V3GraphEdge{m_graphp, fromp, शीर्ष, 1};
    }

    // Iterate the given list of statements, building the dependency graph
    व्योम scanBlock(Ast_गाँठ* stmtsp) {
        अगर (m_noSplitWhy) वापस;
        क्रम (Ast_गाँठ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Skip comments. They have no dependencies at all, so would always
            // form an independent component, and hence a split block, of their
            // own, which would be subsequently deleted as it does nothing.
            अगर (VN_IS(stmtp, Comment)) जारी;
            UASSERT_OBJ(!stmtp->user1p(), stmtp, "user1p should not be set");
            SplitStmtVertex* स्थिर vtxp = नया SplitStmtVertex{m_graphp, stmtp};
            stmtp->user1p(vtxp);
            m_stmtStackps.धकेलो_back(vtxp);
            iterate(stmtp);
            m_stmtStackps.निकालो_back();
        }
    }

    // Remove unnecessary edges, then color to find weakly connected components
    uint32_t colorAlwaysGraph() {
        अगर (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_built", false);

        // Prune duplicate edges. Not necessary for correctness, but simplifies dumps.
        m_graphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        अगर (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nodup", false);

        // Remove variable vertices that are not written in the block.
        // These are input-only to the block, so carry no dependency.
        क्रम (V3GraphVertex* स्थिर vtxp : m_graphp->vertices().unlinkable()) {
            SplitVarStdVertex* स्थिर vstdp = vtxp->cast<SplitVarStdVertex>();
            अगर (!vstdp || !vstdp->outEmpty()) जारी;
            UINFOTREE(9, vstdp->nodep(), "", "Will remove deps on block input var:");
            vstdp->nodep()->user1p(nullptr);  // Don't leave a dangling pointer behind
            VL_DO_DANGLING(vstdp->unlinkDelete(m_graphp), vstdp);
        }
        अगर (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_noinputs", false);

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
        क्रम (V3GraphVertex* स्थिर vtxp : m_graphp->vertices().unlinkable()) {
            SplitStmtVertex* स्थिर stmtVtxp = vtxp->cast<SplitStmtVertex>();
            अगर (!stmtVtxp || !VN_IS(stmtVtxp->nodep(), If)) जारी;
            // Can't remove if dependent on a variable written in the block
            अगर (!stmtVtxp->outEmpty()) जारी;
            // Depends only on block inputs, so can be split. Remove the vertex.
            stmtVtxp->nodep()->user1p(nullptr);
            stmtVtxp->unlinkDelete(m_graphp);
        }
        अगर (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nofreeifs", false);

        // Weak coloring to determine what must stay together in a single block
        स्थिर uint32_t अंक_Colors = m_graphp->weaklyConnected(&V3GraphEdge::followAlwaysTrue);
        अगर (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_colored", false);
        वापस अंक_Colors;
    }

    // Take the statements of the given list, and return them distributed into one list per color.
    अटल std::vector<Ast_गाँठ*> splitStatements(Ast_गाँठ* stmtsp, uint32_t अंक_Colors) {
        std::vector<Ast_गाँठ*> परिणाम{अंक_Colors, nullptr};
        क्रम (Ast_गाँठ *stmtp = stmtsp, *nextp = nullptr; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();  // 'stmtp' is unlinked below

            // Comments are dropped if the block is split
            अगर (VN_IS(stmtp, Comment)) जारी;

            // Pick up the statement vertex, which might be nullptr after pruning during analysis
            स्थिर SplitStmtVertex* स्थिर vtxp = stmtp->user1u().to<SplitStmtVertex*>();

            // If statements are duplicated for each split branch
            अगर (AstIf* स्थिर ifp = VN_CAST(stmtp, If)) {
                स्थिर स्वतः thens = splitStatements(ifp->thensp(), अंक_Colors);
                स्थिर स्वतः elses = splitStatements(ifp->elsesp(), अंक_Colors);
                File_पंक्ति* स्थिर flp = ifp->fileline();
                // Rebuild the 'if' for each color present in either branch
                bool empty = true;
                क्रम (uint32_t color = 0; color < अंक_Colors; ++color) {
                    अगर (!thens[color] && !elses[color]) जारी;
                    empty = false;
                    // The condition is cloned for each color. An impure condition
                    // keeps the 'if' and all it holds in one component, so just once.
                    AstIf* स्थिर clonep = नया AstIf{flp, ifp->condp()->clone_वृक्ष(true),
                                                    thens[color], elses[color]};
                    // Preserve pragmas from unique if's so assertions work properly
                    clonep->uniquePragma(ifp->uniquePragma());
                    clonep->unique0Pragma(ifp->unique0Pragma());
                    clonep->priorityPragma(ifp->priorityPragma());
                    परिणाम[color] = Ast_गाँठ::add_अग्र(परिणाम[color], clonep);
                }
                // There is nothing under the 'if' to guard. If its vertex was
                // removed as having no dependencies at all, then its
                // condition reads only block inputs and is pure, so the whole
                // 'if' can go. Otherwise the condition might have a side
                // effect, so keep just the condition, evaluated as a
                // statement, under the color of the 'if' itself.
                अगर (empty && vtxp) {
                    स्थिर uint32_t color = vtxp->color();
                    Ast_गाँठ_व्यंजक* स्थिर condp = ifp->condp();
                    condp->unlinkFrBack();
                    परिणाम[color] = Ast_गाँठ::add_अग्र(परिणाम[color], नया Ast_Stmt_व्यंजक{flp, condp});
                }
                जारी;
            }

            // Move the leaf into its color's list
            स्थिर uint32_t color = vtxp->color();
            परिणाम[color] = Ast_गाँठ::add_अग्र(परिणाम[color], stmtp->unlinkFrBack());
        }
        वापस परिणाम;
    }

    // VISITORS
    व्योम visit(AstAlways* nodep) override {
        // Skip blocks created below
        अगर (nodep->user3()) वापस;

        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_noSplitWhy);
        VL_RESTORER(m_inDly);
        V3Graph graph;
        m_graphp = &graph;
        m_impureVtxp = nullptr;
        m_noSplitWhy = nullptr;
        m_inDly = false;
        UASSERT_OBJ(m_stmtStackps.empty(), nodep, "Statement stack not empty");

        // Build the graph
        स्थिर VNUser1InUse user1InUse;
        स्थिर VNUser2InUse user2InUse;
        scanBlock(nodep->stmtsp());

        // We might have to give up
        अगर (m_noSplitWhy) {
            UINFO(9, "  NoSplitBlock because " << m_noSplitWhy);
            वापस;
        }

        // Color the graph to identify separable statements
        स्थिर uint32_t अंक_Colors = colorAlwaysGraph();
        // If the whole block is one component (or empty), then nothing to split
        अगर (अंक_Colors <= 1) वापस;

        UINFO(6, "  splitting always " << nodep);

        // Count the number of new blocks inserted into the Ast: '1 -> n' split, so 'n - 1' extra
        m_statSplits += अंक_Colors - 1;

        // Unpick the statements out of the original block, into one list per color
        स्थिर स्वतः lists = splitStatements(nodep->stmtsp(), अंक_Colors);
        UASSERT_OBJ(lists.आकार() == अंक_Colors, nodep, "Inconsistent split");

        // Whatever 'splitStatements' did not take, (comments, empty ifs) is not needed any more
        अगर (Ast_गाँठ* स्थिर restp = nodep->stmtsp()) {
            VL_DO_DANGLING(restp->unlink_Fr_Back_With_अग्र()->delete_वृक्ष(), restp);
        }

        // Every color has a statement in it. Reuse the original block for the
        // first color, and add a new block after it for each of the rest.
        UASSERT_OBJ(lists.front(), nodep, "Color with no statements");
        nodep->addStmtsp(lists.front());
        Ast_गाँठ* lastp = nodep;
        File_पंक्ति* स्थिर flp = nodep->fileline();
        स्थिर VAlwaysKwd kwd = nodep->keyword();
        क्रम (माप_प्रकार i = 1; i < अंक_Colors; ++i) {
            Ast_गाँठ* स्थिर stmtsp = lists[i];
            UASSERT_OBJ(stmtsp, nodep, "Color with no statements");
            AstAlways* स्थिर newp = नया AstAlways{flp, kwd, nullptr, stmtsp};
            newp->user3(1);  // Do not split again
            lastp->add_अग्र_Here(newp);
            lastp = newp;
        }
    }

    व्योम visit(AstIf* nodep) override {
        अगर (!m_graphp || m_noSplitWhy) वापस;
        {
            VL_RESTORER(m_currIfp);
            m_currIfp = nodep;
            iterate_And_अग्र_Null(nodep->condp());
        }
        scanBlock(nodep->thensp());
        scanBlock(nodep->elsesp());
    }

    व्योम visit(Ast_व्यंजक_Stmt* nodep) override {
        अगर (!m_graphp || m_noSplitWhy) वापस;
        VL_RESTORER(m_inDly);
        m_inDly = false;
        iterateChildren(nodep);
    }

    व्योम visit(AstAssignDly* nodep) override {
        अगर (!m_graphp || m_noSplitWhy) वापस;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        अगर (Ast_गाँठ* स्थिर controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_inDly);
        m_inDly = true;
        iterate(nodep->lhsp());
    }

    व्योम visit(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        अगर (m_graphp) m_noSplitWhy = "Handle";
        visit(static_cast<Ast_गाँठ*>(nodep));
    }

    व्योम visit(AstJumpGo*) override {
        अगर (!m_graphp || m_noSplitWhy) वापस;
        m_noSplitWhy = "JumpGo";
    }

    व्योम visit(AstVarRef* nodep) override {
        अगर (!m_graphp || m_noSplitWhy) वापस;
        UASSERT_OBJ(!m_stmtStackps.empty(), nodep, "Not under a statement");

        // Constant lookups can be ignored
        अगर (nodep->varp()->isConst()) वापस;

        AstVarScope* स्थिर vscp = nodep->varScopep();

        // SPEEDUP: We add duplicate edges, that should be fixed
        अगर (m_inDly && nodep->access().isWriteOrRW()) {
            // Delayed variable: is different from non-delayed variable, writes to it
            // are not observable while executing this block (NBA not yet committed),
            // so add only a write edge to a separate 'post' vertex.
            अगर (!vscp->user2p()) vscp->user2p(नया SplitVarPostVertex{m_graphp, vscp});
            SplitVarPostVertex* स्थिर vpostp = vscp->user2u().to<SplitVarPostVertex*>();
            क्रम (SplitStmtVertex* स्थिर vtxp : m_stmtStackps) addEdge(vpostp, vtxp);
        } अन्यथा अगर (nodep->access().isWriteOrRW()) {
            // Regular (non-blocking) write: Need to maintain program-flow order
            अगर (!vscp->user1p()) vscp->user1p(नया SplitVarStdVertex{m_graphp, vscp});
            SplitVarStdVertex* स्थिर vstdp = vscp->user1u().to<SplitVarStdVertex*>();
            क्रम (SplitStmtVertex* स्थिर vtxp : m_stmtStackps) addEdge(vstdp, vtxp);
        } अन्यथा {
            // Regular (non-blocking) read: Need to maintain program-flow order
            अगर (!vscp->user1p()) vscp->user1p(नया SplitVarStdVertex{m_graphp, vscp});
            SplitVarStdVertex* स्थिर vstdp = vscp->user1u().to<SplitVarStdVertex*>();
            क्रम (SplitStmtVertex* स्थिर vtxp : m_stmtStackps) {
                // If this is an if statement it only depends on refs in its
                // own condition only (not those in its branches). For other
                // statements, just record the referene as normal.
                अगर (स्थिर AstIf* स्थिर ifp = VN_CAST(vtxp->nodep(), If)) {
                    अगर (ifp != m_currIfp) जारी;
                }
                addEdge(vtxp, vstdp);
            }
        }
    }

    व्योम visit(Ast_गाँठ* nodep) override {
        // Outside AstAlways, just descend
        अगर (!m_graphp) {
            iterateChildren(nodep);
            वापस;
        }
        // Early exit if decided not to split
        अगर (m_noSplitWhy) वापस;

        UASSERT_OBJ(!m_stmtStackps.empty(), nodep, "Not under a statement");

        // Timing control prevents splitting
        अगर (nodep->isTimingControl()) {
            m_noSplitWhy = "TimingControl";
            वापस;
        }

        // All impure statements must be grouped together.
        अगर (!nodep->isPure()) {
            अगर (!m_impureVtxp) m_impureVtxp = नया SplitImpureVertex{m_graphp, nodep};
            // One edge is enough to find the weakly connected components, but
            // it must point at the impure vertex, so it is an out edge (input
            // dependency) of any enclosing 'if' to prevent pruning.
            क्रम (SplitStmtVertex* स्थिर vtxp : m_stmtStackps) addEdge(vtxp, m_impureVtxp);
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
    explicit SplitVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SplitVisitor() override { V3Stats::addStat("Optimizations, Split always", m_statSplits); }
    VL_UNCOPYABLE(SplitVisitor);

खुला:
    अटल व्योम apply(AstNetlist* nodep) { SplitVisitor{nodep}; }
};

//######################################################################
// Split class functions

व्योम V3Split::splitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    SplitVisitor::apply(nodep);
    V3Global::dump_Check_Global_वृक्ष("split", 0, dump_वृक्ष_Either_Level() >= 3);
}
