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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Split.h"

#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <string>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Support classes

वग्ग Split_ग्रन्थिः_Vertex VL_NOT_FINAL : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(Split_ग्रन्थिः_Vertex, V3GraphVertex)
    Ast_ग्रन्थिः* ध्रुव m_nodep;

रक्षित:
    Split_ग्रन्थिः_Vertex(V3Graph* graphp, Ast_ग्रन्थिः* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    // ACCESSORS
    std::string name() ध्रुव override {
        पटिनिवत्त cvtToHex(m_nodep) + ' ' + m_nodep->prettyTypeName();
    }

सार्वजनिक:
    Ast_ग्रन्थिः* nodep() ध्रुव { पटिनिवत्त m_nodep; }
};

वग्ग SplitImpureVertex final : सार्वजनिक Split_ग्रन्थिः_Vertex {
    VL_RTTI_IMPL(SplitImpureVertex, Split_ग्रन्थिः_Vertex)

    std::string name() ध्रुव override { पटिनिवत्त "*IMPURE*"; }
    std::string dotColor() ध्रुव override { पटिनिवत्त "green"; }

सार्वजनिक:
    explicit SplitImpureVertex(V3Graph* graphp, Ast_ग्रन्थिः* nodep)
        : Split_ग्रन्थिः_Vertex{graphp, nodep} {}
};

वग्ग SplitStmtVertex final : सार्वजनिक Split_ग्रन्थिः_Vertex {
    VL_RTTI_IMPL(SplitStmtVertex, Split_ग्रन्थिः_Vertex)

    std::string dotColor() ध्रुव override { पटिनिवत्त "yellow"; }

सार्वजनिक:
    SplitStmtVertex(V3Graph* graphp, Ast_ग्रन्थिः* nodep)
        : Split_ग्रन्थिः_Vertex{graphp, nodep} {}
};

वग्ग SplitVarStdVertex final : सार्वजनिक Split_ग्रन्थिः_Vertex {
    VL_RTTI_IMPL(SplitVarStdVertex, Split_ग्रन्थिः_Vertex)

    std::string dotColor() ध्रुव override { पटिनिवत्त "skyblue"; }

सार्वजनिक:
    SplitVarStdVertex(V3Graph* graphp, AstVarScope* vscp)
        : Split_ग्रन्थिः_Vertex{graphp, vscp} {}
};

वग्ग SplitVarPostVertex final : सार्वजनिक Split_ग्रन्थिः_Vertex {
    VL_RTTI_IMPL(SplitVarPostVertex, Split_ग्रन्थिः_Vertex)

    std::string name() ध्रुव override { पटिनिवत्त "POST "s + Split_ग्रन्थिः_Vertex::name(); }
    std::string dotColor() ध्रुव override { पटिनिवत्त "CadetBlue"; }

सार्वजनिक:
    SplitVarPostVertex(V3Graph* graphp, AstVarScope* vscp)
        : Split_ग्रन्थिः_Vertex{graphp, vscp} {}
};

वग्ग SplitVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> SplitVarStdVertex*: Regular program-flow variable vertex
    // AstVarScope::user2p  -> SplitVarPostVertex*: NBA written delayed variable vertex
    // Ast{StmtIsh}::user1p -> SplitStmtVertex*

    // NODE STATE
    // AstAlways::user3     -> bool: Block created by splitting, needs no further splitting
    ध्रुव VNUser3InUse m_inuser3;

    // STATE
    V3Graph* m_graphp = nullptr;  // Dependency graph to analyze statement connectivity
    std::vector<SplitStmtVertex*> m_stmtStackps;  // Current statements being tracked
    SplitImpureVertex* m_impureVtxp = nullptr;  // Vertex connecting impure statements
    ध्रुव वर्ण* m_noSplitWhy = nullptr;  // Reason current block cannot be split
    तार्किक m_inDly = मिच्छा;  // Inside AstAssignDly Lhs
    ध्रुव AstIf* m_currIfp = nullptr;  // The AstIf whose condition is currently visited
    VDouble0 m_statSplits;  // Statistic tracking

    // METHODS
    शून्य addEdge(V3GraphVertex* fromp, V3GraphVertex* शिखरम्) {
        नूतन V3GraphEdge{m_graphp, fromp, शिखरम्, 1};
    }

    // Iterate the given list of statements, building the dependency graph
    शून्य scanBlock(Ast_ग्रन्थिः* stmtsp) {
        सचे (m_noSplitWhy) पटिनिवत्त;
        अत्थाय (Ast_ग्रन्थिः* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Skip comments. They have no dependencies at all, so would always
            // form an independent component, and hence a split block, of their
            // own, which would be subsequently deleted as it does nothing.
            सचे (VN_IS(stmtp, Comment)) अनुवर्त;
            UASSERT_OBJ(!stmtp->user1p(), stmtp, "user1p should not be set");
            SplitStmtVertex* ध्रुव vtxp = नूतन SplitStmtVertex{m_graphp, stmtp};
            stmtp->user1p(vtxp);
            m_stmtStackps.प्रक्षिप_back(vtxp);
            iterate(stmtp);
            m_stmtStackps.निष्कासय_back();
        }
    }

    // Remove unnecessary edges, then color to find weakly connected components
    uint32_t colorAlwaysGraph() {
        सचे (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_built", मिच्छा);

        // Prune duplicate edges. Not necessary for correctness, but simplifies dumps.
        m_graphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        सचे (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nodup", मिच्छा);

        // Remove variable vertices that are not written in the block.
        // These are input-only to the block, so carry no dependency.
        अत्थाय (V3GraphVertex* ध्रुव vtxp : m_graphp->vertices().unlinkable()) {
            SplitVarStdVertex* ध्रुव vstdp = vtxp->cast<SplitVarStdVertex>();
            सचे (!vstdp || !vstdp->outEmpty()) अनुवर्त;
            UINFOTREE(9, vstdp->nodep(), "", "Will remove deps on block input var:");
            vstdp->nodep()->user1p(nullptr);  // Don't leave a dangling pointer behind
            VL_DO_DANGLING(vstdp->unlinkDelete(m_graphp), vstdp);
        }
        सचे (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_noinputs", मिच्छा);

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
        अत्थाय (V3GraphVertex* ध्रुव vtxp : m_graphp->vertices().unlinkable()) {
            SplitStmtVertex* ध्रुव stmtVtxp = vtxp->cast<SplitStmtVertex>();
            सचे (!stmtVtxp || !VN_IS(stmtVtxp->nodep(), If)) अनुवर्त;
            // Can't remove if dependent on a variable written in the block
            सचे (!stmtVtxp->outEmpty()) अनुवर्त;
            // Depends only on block inputs, so can be split. Remove the vertex.
            stmtVtxp->nodep()->user1p(nullptr);
            stmtVtxp->unlinkDelete(m_graphp);
        }
        सचे (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nofreeifs", मिच्छा);

        // Weak coloring to determine what must stay together in a single block
        ध्रुव uint32_t अङ्कः_Colors = m_graphp->weaklyConnected(&V3GraphEdge::followAlwaysTrue);
        सचे (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_colored", मिच्छा);
        पटिनिवत्त अङ्कः_Colors;
    }

    // Take the statements of the given list, and return them distributed into one list per color.
    स्थिर std::vector<Ast_ग्रन्थिः*> splitStatements(Ast_ग्रन्थिः* stmtsp, uint32_t अङ्कः_Colors) {
        std::vector<Ast_ग्रन्थिः*> result{अङ्कः_Colors, nullptr};
        अत्थाय (Ast_ग्रन्थिः *stmtp = stmtsp, *nextp = nullptr; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();  // 'stmtp' is unlinked below

            // Comments are dropped if the block is split
            सचे (VN_IS(stmtp, Comment)) अनुवर्त;

            // Pick up the statement vertex, which might be nullptr after pruning during analysis
            ध्रुव SplitStmtVertex* ध्रुव vtxp = stmtp->user1u().to<SplitStmtVertex*>();

            // If statements are duplicated for each split branch
            सचे (AstIf* ध्रुव ifp = VN_CAST(stmtp, If)) {
                ध्रुव auto thens = splitStatements(ifp->thensp(), अङ्कः_Colors);
                ध्रुव auto elses = splitStatements(ifp->elsesp(), अङ्कः_Colors);
                File_पङ्क्तिः* ध्रुव flp = ifp->fileline();
                // Rebuild the 'if' for each color present in either branch
                तार्किक empty = सच्च;
                अत्थाय (uint32_t color = 0; color < अङ्कः_Colors; ++color) {
                    सचे (!thens[color] && !elses[color]) अनुवर्त;
                    empty = मिच्छा;
                    // The condition is cloned for each color. An impure condition
                    // keeps the 'if' and all it holds in one component, so just once.
                    AstIf* ध्रुव clonep = नूतन AstIf{flp, ifp->condp()->clone_वृक्षः(सच्च),
                                                    thens[color], elses[color]};
                    // Preserve pragmas from unique if's so assertions work properly
                    clonep->uniquePragma(ifp->uniquePragma());
                    clonep->unique0Pragma(ifp->unique0Pragma());
                    clonep->priorityPragma(ifp->priorityPragma());
                    result[color] = Ast_ग्रन्थिः::addNext(result[color], clonep);
                }
                // There is nothing under the 'if' to guard. If its vertex was
                // removed as having no dependencies at all, then its
                // condition reads only block inputs and is pure, so the whole
                // 'if' can go. Otherwise the condition might have a side
                // effect, so keep just the condition, evaluated as a
                // statement, under the color of the 'if' itself.
                सचे (empty && vtxp) {
                    ध्रुव uint32_t color = vtxp->color();
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = ifp->condp();
                    condp->unlinkFrBack();
                    result[color] = Ast_ग्रन्थिः::addNext(result[color], नूतन Ast_Stmt_व्यञ्जकम्{flp, condp});
                }
                अनुवर्त;
            }

            // Move the leaf into its color's list
            ध्रुव uint32_t color = vtxp->color();
            result[color] = Ast_ग्रन्थिः::addNext(result[color], stmtp->unlinkFrBack());
        }
        पटिनिवत्त result;
    }

    // VISITORS
    शून्य visit(AstAlways* nodep) override {
        // Skip blocks created below
        सचे (nodep->user3()) पटिनिवत्त;

        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_noSplitWhy);
        VL_RESTORER(m_inDly);
        V3Graph graph;
        m_graphp = &graph;
        m_impureVtxp = nullptr;
        m_noSplitWhy = nullptr;
        m_inDly = मिच्छा;
        UASSERT_OBJ(m_stmtStackps.empty(), nodep, "Statement stack not empty");

        // Build the graph
        ध्रुव VNUser1InUse user1InUse;
        ध्रुव VNUser2InUse user2InUse;
        scanBlock(nodep->stmtsp());

        // We might have to give up
        सचे (m_noSplitWhy) {
            UINFO(9, "  NoSplitBlock because " << m_noSplitWhy);
            पटिनिवत्त;
        }

        // Color the graph to identify separable statements
        ध्रुव uint32_t अङ्कः_Colors = colorAlwaysGraph();
        // If the whole block is one component (or empty), then nothing to split
        सचे (अङ्कः_Colors <= 1) पटिनिवत्त;

        UINFO(6, "  splitting always " << nodep);

        // Count the number of new blocks inserted into the Ast: '1 -> n' split, so 'n - 1' extra
        m_statSplits += अङ्कः_Colors - 1;

        // Unpick the statements out of the original block, into one list per color
        ध्रुव auto lists = splitStatements(nodep->stmtsp(), अङ्कः_Colors);
        UASSERT_OBJ(lists.size() == अङ्कः_Colors, nodep, "Inconsistent split");

        // Whatever 'splitStatements' did not take, (comments, empty ifs) is not needed any more
        सचे (Ast_ग्रन्थिः* ध्रुव restp = nodep->stmtsp()) {
            VL_DO_DANGLING(restp->unlinkFrBackWithNext()->delete_वृक्षः(), restp);
        }

        // Every color has a statement in it. Reuse the original block for the
        // first color, and add a new block after it for each of the rest.
        UASSERT_OBJ(lists.front(), nodep, "Color with no statements");
        nodep->addStmtsp(lists.front());
        Ast_ग्रन्थिः* lastp = nodep;
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        ध्रुव VAlwaysKwd kwd = nodep->keyword();
        अत्थाय (size_t i = 1; i < अङ्कः_Colors; ++i) {
            Ast_ग्रन्थिः* ध्रुव stmtsp = lists[i];
            UASSERT_OBJ(stmtsp, nodep, "Color with no statements");
            AstAlways* ध्रुव newp = नूतन AstAlways{flp, kwd, nullptr, stmtsp};
            newp->user3(1);  // Do not split again
            lastp->addNextHere(newp);
            lastp = newp;
        }
    }

    शून्य visit(AstIf* nodep) override {
        सचे (!m_graphp || m_noSplitWhy) पटिनिवत्त;
        {
            VL_RESTORER(m_currIfp);
            m_currIfp = nodep;
            iterateAndNextNull(nodep->condp());
        }
        scanBlock(nodep->thensp());
        scanBlock(nodep->elsesp());
    }

    शून्य visit(Ast_व्यञ्जकम्_Stmt* nodep) override {
        सचे (!m_graphp || m_noSplitWhy) पटिनिवत्त;
        VL_RESTORER(m_inDly);
        m_inDly = मिच्छा;
        iterateChildren(nodep);
    }

    शून्य visit(AstAssignDly* nodep) override {
        सचे (!m_graphp || m_noSplitWhy) पटिनिवत्त;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        सचे (Ast_ग्रन्थिः* ध्रुव controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_inDly);
        m_inDly = सच्च;
        iterate(nodep->lhsp());
    }

    शून्य visit(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        सचे (m_graphp) m_noSplitWhy = "Handle";
        visit(static_cast<Ast_ग्रन्थिः*>(nodep));
    }

    शून्य visit(AstJumpGo*) override {
        सचे (!m_graphp || m_noSplitWhy) पटिनिवत्त;
        m_noSplitWhy = "JumpGo";
    }

    शून्य visit(AstVarRef* nodep) override {
        सचे (!m_graphp || m_noSplitWhy) पटिनिवत्त;
        UASSERT_OBJ(!m_stmtStackps.empty(), nodep, "Not under a statement");

        // Constant lookups can be ignored
        सचे (nodep->varp()->isConst()) पटिनिवत्त;

        AstVarScope* ध्रुव vscp = nodep->varScopep();

        // SPEEDUP: We add duplicate edges, that should be fixed
        सचे (m_inDly && nodep->access().isWriteOrRW()) {
            // Delayed variable: is different from non-delayed variable, writes to it
            // are not observable while executing this block (NBA not yet committed),
            // so add only a write edge to a separate 'post' vertex.
            सचे (!vscp->user2p()) vscp->user2p(नूतन SplitVarPostVertex{m_graphp, vscp});
            SplitVarPostVertex* ध्रुव vpostp = vscp->user2u().to<SplitVarPostVertex*>();
            अत्थाय (SplitStmtVertex* ध्रुव vtxp : m_stmtStackps) addEdge(vpostp, vtxp);
        } अञ्ञथा सचे (nodep->access().isWriteOrRW()) {
            // Regular (non-blocking) write: Need to maintain program-flow order
            सचे (!vscp->user1p()) vscp->user1p(नूतन SplitVarStdVertex{m_graphp, vscp});
            SplitVarStdVertex* ध्रुव vstdp = vscp->user1u().to<SplitVarStdVertex*>();
            अत्थाय (SplitStmtVertex* ध्रुव vtxp : m_stmtStackps) addEdge(vstdp, vtxp);
        } अञ्ञथा {
            // Regular (non-blocking) read: Need to maintain program-flow order
            सचे (!vscp->user1p()) vscp->user1p(नूतन SplitVarStdVertex{m_graphp, vscp});
            SplitVarStdVertex* ध्रुव vstdp = vscp->user1u().to<SplitVarStdVertex*>();
            अत्थाय (SplitStmtVertex* ध्रुव vtxp : m_stmtStackps) {
                // If this is an if statement it only depends on refs in its
                // own condition only (not those in its branches). For other
                // statements, just record the referene as normal.
                सचे (ध्रुव AstIf* ध्रुव ifp = VN_CAST(vtxp->nodep(), If)) {
                    सचे (ifp != m_currIfp) अनुवर्त;
                }
                addEdge(vtxp, vstdp);
            }
        }
    }

    शून्य visit(Ast_ग्रन्थिः* nodep) override {
        // Outside AstAlways, just descend
        सचे (!m_graphp) {
            iterateChildren(nodep);
            पटिनिवत्त;
        }
        // Early exit if decided not to split
        सचे (m_noSplitWhy) पटिनिवत्त;

        UASSERT_OBJ(!m_stmtStackps.empty(), nodep, "Not under a statement");

        // Timing control prevents splitting
        सचे (nodep->isTimingControl()) {
            m_noSplitWhy = "TimingControl";
            पटिनिवत्त;
        }

        // All impure statements must be grouped together.
        सचे (!nodep->isPure()) {
            सचे (!m_impureVtxp) m_impureVtxp = नूतन SplitImpureVertex{m_graphp, nodep};
            // One edge is enough to find the weakly connected components, but
            // it must point at the impure vertex, so it is an out edge (input
            // dependency) of any enclosing 'if' to prevent pruning.
            अत्थाय (SplitStmtVertex* ध्रुव vtxp : m_stmtStackps) addEdge(vtxp, m_impureVtxp);
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
    explicit SplitVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SplitVisitor() override { V3Stats::addStat("Optimizations, Split always", m_statSplits); }
    VL_UNCOPYABLE(SplitVisitor);

सार्वजनिक:
    स्थिर शून्य apply(AstNetlist* nodep) { SplitVisitor{nodep}; }
};

//######################################################################
// Split class functions

शून्य V3Split::splitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    SplitVisitor::apply(nodep);
    V3Global::dump_Check_Global_वृक्षः("split", 0, dump_वृक्षः_Either_Level() >= 3);
}
