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

वर्ग Split_गाठ_Vertex VL_NOT_FINAL : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(Split_गाठ_Vertex, V3GraphVertex)
    Ast_गाठ* स्थिरांक m_nodep;

संरक्षित:
    Split_गाठ_Vertex(V3Graph* graphp, Ast_गाठ* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    // ACCESSORS
    std::string name() स्थिरांक override {
        परत cvtToHex(m_nodep) + ' ' + m_nodep->prettyTypeName();
    }

सार्वजनिक:
    Ast_गाठ* nodep() स्थिरांक { परत m_nodep; }
};

वर्ग SplitImpureVertex final : सार्वजनिक Split_गाठ_Vertex {
    VL_RTTI_IMPL(SplitImpureVertex, Split_गाठ_Vertex)

    std::string name() स्थिरांक override { परत "*IMPURE*"; }
    std::string dotColor() स्थिरांक override { परत "green"; }

सार्वजनिक:
    explicit SplitImpureVertex(V3Graph* graphp, Ast_गाठ* nodep)
        : Split_गाठ_Vertex{graphp, nodep} {}
};

वर्ग SplitStmtVertex final : सार्वजनिक Split_गाठ_Vertex {
    VL_RTTI_IMPL(SplitStmtVertex, Split_गाठ_Vertex)

    std::string dotColor() स्थिरांक override { परत "yellow"; }

सार्वजनिक:
    SplitStmtVertex(V3Graph* graphp, Ast_गाठ* nodep)
        : Split_गाठ_Vertex{graphp, nodep} {}
};

वर्ग SplitVarStdVertex final : सार्वजनिक Split_गाठ_Vertex {
    VL_RTTI_IMPL(SplitVarStdVertex, Split_गाठ_Vertex)

    std::string dotColor() स्थिरांक override { परत "skyblue"; }

सार्वजनिक:
    SplitVarStdVertex(V3Graph* graphp, AstVarScope* vscp)
        : Split_गाठ_Vertex{graphp, vscp} {}
};

वर्ग SplitVarPostVertex final : सार्वजनिक Split_गाठ_Vertex {
    VL_RTTI_IMPL(SplitVarPostVertex, Split_गाठ_Vertex)

    std::string name() स्थिरांक override { परत "POST "s + Split_गाठ_Vertex::name(); }
    std::string dotColor() स्थिरांक override { परत "CadetBlue"; }

सार्वजनिक:
    SplitVarPostVertex(V3Graph* graphp, AstVarScope* vscp)
        : Split_गाठ_Vertex{graphp, vscp} {}
};

वर्ग SplitVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> SplitVarStdVertex*: Regular program-flow variable vertex
    // AstVarScope::user2p  -> SplitVarPostVertex*: NBA written delayed variable vertex
    // Ast{StmtIsh}::user1p -> SplitStmtVertex*

    // NODE STATE
    // AstAlways::user3     -> bool: Block created by splitting, needs no further splitting
    स्थिरांक VNUser3InUse m_inuser3;

    // STATE
    V3Graph* m_graphp = nullptr;  // Dependency graph to analyze statement connectivity
    std::vector<SplitStmtVertex*> m_stmtStackps;  // Current statements being tracked
    SplitImpureVertex* m_impureVtxp = nullptr;  // Vertex connecting impure statements
    स्थिरांक अक्षर* m_noSplitWhy = nullptr;  // Reason current block cannot be split
    तार्किक m_inDly = असत्य;  // Inside AstAssignDly Lhs
    स्थिरांक AstIf* m_currIfp = nullptr;  // The AstIf whose condition is currently visited
    VDouble0 m_statSplits;  // Statistic tracking

    // METHODS
    शून्य addEdge(V3GraphVertex* fromp, V3GraphVertex* शीर्ष) {
        नवीन V3GraphEdge{m_graphp, fromp, शीर्ष, 1};
    }

    // Iterate the given list of statements, building the dependency graph
    शून्य scanBlock(Ast_गाठ* stmtsp) {
        जर (m_noSplitWhy) परत;
        साठी (Ast_गाठ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Skip comments. They have no dependencies at all, so would always
            // form an independent component, and hence a split block, of their
            // own, which would be subsequently deleted as it does nothing.
            जर (VN_IS(stmtp, Comment)) सुरूठेवा;
            UASSERT_OBJ(!stmtp->user1p(), stmtp, "user1p should not be set");
            SplitStmtVertex* स्थिरांक vtxp = नवीन SplitStmtVertex{m_graphp, stmtp};
            stmtp->user1p(vtxp);
            m_stmtStackps.ढकला_back(vtxp);
            iterate(stmtp);
            m_stmtStackps.काढा_back();
        }
    }

    // Remove unnecessary edges, then color to find weakly connected components
    uint32_t colorAlwaysGraph() {
        जर (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_built", असत्य);

        // Prune duplicate edges. Not necessary for correctness, but simplifies dumps.
        m_graphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        जर (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nodup", असत्य);

        // Remove variable vertices that are not written in the block.
        // These are input-only to the block, so carry no dependency.
        साठी (V3GraphVertex* स्थिरांक vtxp : m_graphp->vertices().unlinkable()) {
            SplitVarStdVertex* स्थिरांक vstdp = vtxp->cast<SplitVarStdVertex>();
            जर (!vstdp || !vstdp->outEmpty()) सुरूठेवा;
            UINFOTREE(9, vstdp->nodep(), "", "Will remove deps on block input var:");
            vstdp->nodep()->user1p(nullptr);  // Don't leave a dangling pointer behind
            VL_DO_DANGLING(vstdp->unlinkDelete(m_graphp), vstdp);
        }
        जर (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_noinputs", असत्य);

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
        साठी (V3GraphVertex* स्थिरांक vtxp : m_graphp->vertices().unlinkable()) {
            SplitStmtVertex* स्थिरांक stmtVtxp = vtxp->cast<SplitStmtVertex>();
            जर (!stmtVtxp || !VN_IS(stmtVtxp->nodep(), If)) सुरूठेवा;
            // Can't remove if dependent on a variable written in the block
            जर (!stmtVtxp->outEmpty()) सुरूठेवा;
            // Depends only on block inputs, so can be split. Remove the vertex.
            stmtVtxp->nodep()->user1p(nullptr);
            stmtVtxp->unlinkDelete(m_graphp);
        }
        जर (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nofreeifs", असत्य);

        // Weak coloring to determine what must stay together in a single block
        स्थिरांक uint32_t अंक_Colors = m_graphp->weaklyConnected(&V3GraphEdge::followAlwaysTrue);
        जर (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_colored", असत्य);
        परत अंक_Colors;
    }

    // Take the statements of the given list, and return them distributed into one list per color.
    स्थिर std::vector<Ast_गाठ*> splitStatements(Ast_गाठ* stmtsp, uint32_t अंक_Colors) {
        std::vector<Ast_गाठ*> result{अंक_Colors, nullptr};
        साठी (Ast_गाठ *stmtp = stmtsp, *nextp = nullptr; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();  // 'stmtp' is unlinked below

            // Comments are dropped if the block is split
            जर (VN_IS(stmtp, Comment)) सुरूठेवा;

            // Pick up the statement vertex, which might be nullptr after pruning during analysis
            स्थिरांक SplitStmtVertex* स्थिरांक vtxp = stmtp->user1u().to<SplitStmtVertex*>();

            // If statements are duplicated for each split branch
            जर (AstIf* स्थिरांक ifp = VN_CAST(stmtp, If)) {
                स्थिरांक स्वतः thens = splitStatements(ifp->thensp(), अंक_Colors);
                स्थिरांक स्वतः elses = splitStatements(ifp->elsesp(), अंक_Colors);
                File_ओळ* स्थिरांक flp = ifp->fileline();
                // Rebuild the 'if' for each color present in either branch
                तार्किक empty = सत्य;
                साठी (uint32_t color = 0; color < अंक_Colors; ++color) {
                    जर (!thens[color] && !elses[color]) सुरूठेवा;
                    empty = असत्य;
                    // The condition is cloned for each color. An impure condition
                    // keeps the 'if' and all it holds in one component, so just once.
                    AstIf* स्थिरांक clonep = नवीन AstIf{flp, ifp->condp()->clone_वृक्ष(सत्य),
                                                    thens[color], elses[color]};
                    // Preserve pragmas from unique if's so assertions work properly
                    clonep->uniquePragma(ifp->uniquePragma());
                    clonep->unique0Pragma(ifp->unique0Pragma());
                    clonep->priorityPragma(ifp->priorityPragma());
                    result[color] = Ast_गाठ::addNext(result[color], clonep);
                }
                // There is nothing under the 'if' to guard. If its vertex was
                // removed as having no dependencies at all, then its
                // condition reads only block inputs and is pure, so the whole
                // 'if' can go. Otherwise the condition might have a side
                // effect, so keep just the condition, evaluated as a
                // statement, under the color of the 'if' itself.
                जर (empty && vtxp) {
                    स्थिरांक uint32_t color = vtxp->color();
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक condp = ifp->condp();
                    condp->unlinkFrBack();
                    result[color] = Ast_गाठ::addNext(result[color], नवीन Ast_Stmt_अभिव्यक्ती{flp, condp});
                }
                सुरूठेवा;
            }

            // Move the leaf into its color's list
            स्थिरांक uint32_t color = vtxp->color();
            result[color] = Ast_गाठ::addNext(result[color], stmtp->unlinkFrBack());
        }
        परत result;
    }

    // VISITORS
    शून्य visit(AstAlways* nodep) override {
        // Skip blocks created below
        जर (nodep->user3()) परत;

        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_noSplitWhy);
        VL_RESTORER(m_inDly);
        V3Graph graph;
        m_graphp = &graph;
        m_impureVtxp = nullptr;
        m_noSplitWhy = nullptr;
        m_inDly = असत्य;
        UASSERT_OBJ(m_stmtStackps.empty(), nodep, "Statement stack not empty");

        // Build the graph
        स्थिरांक VNUser1InUse user1InUse;
        स्थिरांक VNUser2InUse user2InUse;
        scanBlock(nodep->stmtsp());

        // We might have to give up
        जर (m_noSplitWhy) {
            UINFO(9, "  NoSplitBlock because " << m_noSplitWhy);
            परत;
        }

        // Color the graph to identify separable statements
        स्थिरांक uint32_t अंक_Colors = colorAlwaysGraph();
        // If the whole block is one component (or empty), then nothing to split
        जर (अंक_Colors <= 1) परत;

        UINFO(6, "  splitting always " << nodep);

        // Count the number of new blocks inserted into the Ast: '1 -> n' split, so 'n - 1' extra
        m_statSplits += अंक_Colors - 1;

        // Unpick the statements out of the original block, into one list per color
        स्थिरांक स्वतः lists = splitStatements(nodep->stmtsp(), अंक_Colors);
        UASSERT_OBJ(lists.size() == अंक_Colors, nodep, "Inconsistent split");

        // Whatever 'splitStatements' did not take, (comments, empty ifs) is not needed any more
        जर (Ast_गाठ* स्थिरांक restp = nodep->stmtsp()) {
            VL_DO_DANGLING(restp->unlinkFrBackWithNext()->delete_वृक्ष(), restp);
        }

        // Every color has a statement in it. Reuse the original block for the
        // first color, and add a new block after it for each of the rest.
        UASSERT_OBJ(lists.front(), nodep, "Color with no statements");
        nodep->addStmtsp(lists.front());
        Ast_गाठ* lastp = nodep;
        File_ओळ* स्थिरांक flp = nodep->fileline();
        स्थिरांक VAlwaysKwd kwd = nodep->keyword();
        साठी (माप_प्रकार i = 1; i < अंक_Colors; ++i) {
            Ast_गाठ* स्थिरांक stmtsp = lists[i];
            UASSERT_OBJ(stmtsp, nodep, "Color with no statements");
            AstAlways* स्थिरांक newp = नवीन AstAlways{flp, kwd, nullptr, stmtsp};
            newp->user3(1);  // Do not split again
            lastp->addNextHere(newp);
            lastp = newp;
        }
    }

    शून्य visit(AstIf* nodep) override {
        जर (!m_graphp || m_noSplitWhy) परत;
        {
            VL_RESTORER(m_currIfp);
            m_currIfp = nodep;
            iterateAndNextNull(nodep->condp());
        }
        scanBlock(nodep->thensp());
        scanBlock(nodep->elsesp());
    }

    शून्य visit(Ast_अभिव्यक्ती_Stmt* nodep) override {
        जर (!m_graphp || m_noSplitWhy) परत;
        VL_RESTORER(m_inDly);
        m_inDly = असत्य;
        iterateChildren(nodep);
    }

    शून्य visit(AstAssignDly* nodep) override {
        जर (!m_graphp || m_noSplitWhy) परत;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        जर (Ast_गाठ* स्थिरांक controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_inDly);
        m_inDly = सत्य;
        iterate(nodep->lhsp());
    }

    शून्य visit(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        जर (m_graphp) m_noSplitWhy = "Handle";
        visit(static_cast<Ast_गाठ*>(nodep));
    }

    शून्य visit(AstJumpGo*) override {
        जर (!m_graphp || m_noSplitWhy) परत;
        m_noSplitWhy = "JumpGo";
    }

    शून्य visit(AstVarRef* nodep) override {
        जर (!m_graphp || m_noSplitWhy) परत;
        UASSERT_OBJ(!m_stmtStackps.empty(), nodep, "Not under a statement");

        // Constant lookups can be ignored
        जर (nodep->varp()->isConst()) परत;

        AstVarScope* स्थिरांक vscp = nodep->varScopep();

        // SPEEDUP: We add duplicate edges, that should be fixed
        जर (m_inDly && nodep->access().isWriteOrRW()) {
            // Delayed variable: is different from non-delayed variable, writes to it
            // are not observable while executing this block (NBA not yet committed),
            // so add only a write edge to a separate 'post' vertex.
            जर (!vscp->user2p()) vscp->user2p(नवीन SplitVarPostVertex{m_graphp, vscp});
            SplitVarPostVertex* स्थिरांक vpostp = vscp->user2u().to<SplitVarPostVertex*>();
            साठी (SplitStmtVertex* स्थिरांक vtxp : m_stmtStackps) addEdge(vpostp, vtxp);
        } अन्यथा जर (nodep->access().isWriteOrRW()) {
            // Regular (non-blocking) write: Need to maintain program-flow order
            जर (!vscp->user1p()) vscp->user1p(नवीन SplitVarStdVertex{m_graphp, vscp});
            SplitVarStdVertex* स्थिरांक vstdp = vscp->user1u().to<SplitVarStdVertex*>();
            साठी (SplitStmtVertex* स्थिरांक vtxp : m_stmtStackps) addEdge(vstdp, vtxp);
        } अन्यथा {
            // Regular (non-blocking) read: Need to maintain program-flow order
            जर (!vscp->user1p()) vscp->user1p(नवीन SplitVarStdVertex{m_graphp, vscp});
            SplitVarStdVertex* स्थिरांक vstdp = vscp->user1u().to<SplitVarStdVertex*>();
            साठी (SplitStmtVertex* स्थिरांक vtxp : m_stmtStackps) {
                // If this is an if statement it only depends on refs in its
                // own condition only (not those in its branches). For other
                // statements, just record the referene as normal.
                जर (स्थिरांक AstIf* स्थिरांक ifp = VN_CAST(vtxp->nodep(), If)) {
                    जर (ifp != m_currIfp) सुरूठेवा;
                }
                addEdge(vtxp, vstdp);
            }
        }
    }

    शून्य visit(Ast_गाठ* nodep) override {
        // Outside AstAlways, just descend
        जर (!m_graphp) {
            iterateChildren(nodep);
            परत;
        }
        // Early exit if decided not to split
        जर (m_noSplitWhy) परत;

        UASSERT_OBJ(!m_stmtStackps.empty(), nodep, "Not under a statement");

        // Timing control prevents splitting
        जर (nodep->isTimingControl()) {
            m_noSplitWhy = "TimingControl";
            परत;
        }

        // All impure statements must be grouped together.
        जर (!nodep->isPure()) {
            जर (!m_impureVtxp) m_impureVtxp = नवीन SplitImpureVertex{m_graphp, nodep};
            // One edge is enough to find the weakly connected components, but
            // it must point at the impure vertex, so it is an out edge (input
            // dependency) of any enclosing 'if' to prevent pruning.
            साठी (SplitStmtVertex* स्थिरांक vtxp : m_stmtStackps) addEdge(vtxp, m_impureVtxp);
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
    V3Global::dump_Check_Global_वृक्ष("split", 0, dump_वृक्ष_Either_Level() >= 3);
}
