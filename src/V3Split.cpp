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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Split.h"

#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <string>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Support classes

வகுப்பு Split_முனை_Vertex VL_NOT_FINAL : பொது V3GraphVertex {
    VL_RTTI_IMPL(Split_முனை_Vertex, V3GraphVertex)
    Ast_முனை* மாறிலி m_nodep;

பாதுகாக்கப்பட்ட:
    Split_முனை_Vertex(V3Graph* graphp, Ast_முனை* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    // ACCESSORS
    std::string name() மாறிலி override {
        திரும்பு cvtToHex(m_nodep) + ' ' + m_nodep->prettyTypeName();
    }

பொது:
    Ast_முனை* nodep() மாறிலி { திரும்பு m_nodep; }
};

வகுப்பு SplitImpureVertex final : பொது Split_முனை_Vertex {
    VL_RTTI_IMPL(SplitImpureVertex, Split_முனை_Vertex)

    std::string name() மாறிலி override { திரும்பு "*IMPURE*"; }
    std::string dotColor() மாறிலி override { திரும்பு "green"; }

பொது:
    explicit SplitImpureVertex(V3Graph* graphp, Ast_முனை* nodep)
        : Split_முனை_Vertex{graphp, nodep} {}
};

வகுப்பு SplitStmtVertex final : பொது Split_முனை_Vertex {
    VL_RTTI_IMPL(SplitStmtVertex, Split_முனை_Vertex)

    std::string dotColor() மாறிலி override { திரும்பு "yellow"; }

பொது:
    SplitStmtVertex(V3Graph* graphp, Ast_முனை* nodep)
        : Split_முனை_Vertex{graphp, nodep} {}
};

வகுப்பு SplitVarStdVertex final : பொது Split_முனை_Vertex {
    VL_RTTI_IMPL(SplitVarStdVertex, Split_முனை_Vertex)

    std::string dotColor() மாறிலி override { திரும்பு "skyblue"; }

பொது:
    SplitVarStdVertex(V3Graph* graphp, AstVarScope* vscp)
        : Split_முனை_Vertex{graphp, vscp} {}
};

வகுப்பு SplitVarPostVertex final : பொது Split_முனை_Vertex {
    VL_RTTI_IMPL(SplitVarPostVertex, Split_முனை_Vertex)

    std::string name() மாறிலி override { திரும்பு "POST "s + Split_முனை_Vertex::name(); }
    std::string dotColor() மாறிலி override { திரும்பு "CadetBlue"; }

பொது:
    SplitVarPostVertex(V3Graph* graphp, AstVarScope* vscp)
        : Split_முனை_Vertex{graphp, vscp} {}
};

வகுப்பு SplitVisitor final : பொது VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> SplitVarStdVertex*: Regular program-flow variable vertex
    // AstVarScope::user2p  -> SplitVarPostVertex*: NBA written delayed variable vertex
    // Ast{StmtIsh}::user1p -> SplitStmtVertex*

    // NODE STATE
    // AstAlways::user3     -> bool: Block created by splitting, needs no further splitting
    மாறிலி VNUser3InUse m_inuser3;

    // STATE
    V3Graph* m_graphp = nullptr;  // Dependency graph to analyze statement connectivity
    std::vector<SplitStmtVertex*> m_stmtStackps;  // Current statements being tracked
    SplitImpureVertex* m_impureVtxp = nullptr;  // Vertex connecting impure statements
    மாறிலி எழுத்து* m_noSplitWhy = nullptr;  // Reason current block cannot be split
    தருக்க m_inDly = பொய்;  // Inside AstAssignDly Lhs
    மாறிலி AstIf* m_currIfp = nullptr;  // The AstIf whose condition is currently visited
    VDouble0 m_statSplits;  // Statistic tracking

    // METHODS
    வெறுமை addEdge(V3GraphVertex* fromp, V3GraphVertex* உச்சி) {
        புதிய V3GraphEdge{m_graphp, fromp, உச்சி, 1};
    }

    // Iterate the given list of statements, building the dependency graph
    வெறுமை scanBlock(Ast_முனை* stmtsp) {
        எனில் (m_noSplitWhy) திரும்பு;
        க்கு (Ast_முனை* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Skip comments. They have no dependencies at all, so would always
            // form an independent component, and hence a split block, of their
            // own, which would be subsequently deleted as it does nothing.
            எனில் (VN_IS(stmtp, Comment)) தொடர்;
            UASSERT_OBJ(!stmtp->user1p(), stmtp, "user1p should not be set");
            SplitStmtVertex* மாறிலி vtxp = புதிய SplitStmtVertex{m_graphp, stmtp};
            stmtp->user1p(vtxp);
            m_stmtStackps.தள்ளு_back(vtxp);
            iterate(stmtp);
            m_stmtStackps.எடு_back();
        }
    }

    // Remove unnecessary edges, then color to find weakly connected components
    uint32_t colorAlwaysGraph() {
        எனில் (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_built", பொய்);

        // Prune duplicate edges. Not necessary for correctness, but simplifies dumps.
        m_graphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        எனில் (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nodup", பொய்);

        // Remove variable vertices that are not written in the block.
        // These are input-only to the block, so carry no dependency.
        க்கு (V3GraphVertex* மாறிலி vtxp : m_graphp->vertices().unlinkable()) {
            SplitVarStdVertex* மாறிலி vstdp = vtxp->cast<SplitVarStdVertex>();
            எனில் (!vstdp || !vstdp->outEmpty()) தொடர்;
            UINFOTREE(9, vstdp->nodep(), "", "Will remove deps on block input var:");
            vstdp->nodep()->user1p(nullptr);  // Don't leave a dangling pointer behind
            VL_DO_DANGLING(vstdp->unlinkDelete(m_graphp), vstdp);
        }
        எனில் (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_noinputs", பொய்);

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
        க்கு (V3GraphVertex* மாறிலி vtxp : m_graphp->vertices().unlinkable()) {
            SplitStmtVertex* மாறிலி stmtVtxp = vtxp->cast<SplitStmtVertex>();
            எனில் (!stmtVtxp || !VN_IS(stmtVtxp->nodep(), If)) தொடர்;
            // Can't remove if dependent on a variable written in the block
            எனில் (!stmtVtxp->outEmpty()) தொடர்;
            // Depends only on block inputs, so can be split. Remove the vertex.
            stmtVtxp->nodep()->user1p(nullptr);
            stmtVtxp->unlinkDelete(m_graphp);
        }
        எனில் (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nofreeifs", பொய்);

        // Weak coloring to determine what must stay together in a single block
        மாறிலி uint32_t இலக்கம்_Colors = m_graphp->weaklyConnected(&V3GraphEdge::followAlwaysTrue);
        எனில் (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_colored", பொய்);
        திரும்பு இலக்கம்_Colors;
    }

    // Take the statements of the given list, and return them distributed into one list per color.
    நிலையான std::vector<Ast_முனை*> splitStatements(Ast_முனை* stmtsp, uint32_t இலக்கம்_Colors) {
        std::vector<Ast_முனை*> result{இலக்கம்_Colors, nullptr};
        க்கு (Ast_முனை *stmtp = stmtsp, *nextp = nullptr; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();  // 'stmtp' is unlinked below

            // Comments are dropped if the block is split
            எனில் (VN_IS(stmtp, Comment)) தொடர்;

            // Pick up the statement vertex, which might be nullptr after pruning during analysis
            மாறிலி SplitStmtVertex* மாறிலி vtxp = stmtp->user1u().to<SplitStmtVertex*>();

            // If statements are duplicated for each split branch
            எனில் (AstIf* மாறிலி ifp = VN_CAST(stmtp, If)) {
                மாறிலி auto thens = splitStatements(ifp->thensp(), இலக்கம்_Colors);
                மாறிலி auto elses = splitStatements(ifp->elsesp(), இலக்கம்_Colors);
                File_வரி* மாறிலி flp = ifp->fileline();
                // Rebuild the 'if' for each color present in either branch
                தருக்க empty = உண்மை;
                க்கு (uint32_t color = 0; color < இலக்கம்_Colors; ++color) {
                    எனில் (!thens[color] && !elses[color]) தொடர்;
                    empty = பொய்;
                    // The condition is cloned for each color. An impure condition
                    // keeps the 'if' and all it holds in one component, so just once.
                    AstIf* மாறிலி clonep = புதிய AstIf{flp, ifp->condp()->clone_மரம்(உண்மை),
                                                    thens[color], elses[color]};
                    // Preserve pragmas from unique if's so assertions work properly
                    clonep->uniquePragma(ifp->uniquePragma());
                    clonep->unique0Pragma(ifp->unique0Pragma());
                    clonep->priorityPragma(ifp->priorityPragma());
                    result[color] = Ast_முனை::addNext(result[color], clonep);
                }
                // There is nothing under the 'if' to guard. If its vertex was
                // removed as having no dependencies at all, then its
                // condition reads only block inputs and is pure, so the whole
                // 'if' can go. Otherwise the condition might have a side
                // effect, so keep just the condition, evaluated as a
                // statement, under the color of the 'if' itself.
                எனில் (empty && vtxp) {
                    மாறிலி uint32_t color = vtxp->color();
                    Ast_முனை_கோவை* மாறிலி condp = ifp->condp();
                    condp->unlinkFrBack();
                    result[color] = Ast_முனை::addNext(result[color], புதிய Ast_Stmt_கோவை{flp, condp});
                }
                தொடர்;
            }

            // Move the leaf into its color's list
            மாறிலி uint32_t color = vtxp->color();
            result[color] = Ast_முனை::addNext(result[color], stmtp->unlinkFrBack());
        }
        திரும்பு result;
    }

    // VISITORS
    வெறுமை visit(AstAlways* nodep) override {
        // Skip blocks created below
        எனில் (nodep->user3()) திரும்பு;

        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_noSplitWhy);
        VL_RESTORER(m_inDly);
        V3Graph graph;
        m_graphp = &graph;
        m_impureVtxp = nullptr;
        m_noSplitWhy = nullptr;
        m_inDly = பொய்;
        UASSERT_OBJ(m_stmtStackps.empty(), nodep, "Statement stack not empty");

        // Build the graph
        மாறிலி VNUser1InUse user1InUse;
        மாறிலி VNUser2InUse user2InUse;
        scanBlock(nodep->stmtsp());

        // We might have to give up
        எனில் (m_noSplitWhy) {
            UINFO(9, "  NoSplitBlock because " << m_noSplitWhy);
            திரும்பு;
        }

        // Color the graph to identify separable statements
        மாறிலி uint32_t இலக்கம்_Colors = colorAlwaysGraph();
        // If the whole block is one component (or empty), then nothing to split
        எனில் (இலக்கம்_Colors <= 1) திரும்பு;

        UINFO(6, "  splitting always " << nodep);

        // Count the number of new blocks inserted into the Ast: '1 -> n' split, so 'n - 1' extra
        m_statSplits += இலக்கம்_Colors - 1;

        // Unpick the statements out of the original block, into one list per color
        மாறிலி auto lists = splitStatements(nodep->stmtsp(), இலக்கம்_Colors);
        UASSERT_OBJ(lists.size() == இலக்கம்_Colors, nodep, "Inconsistent split");

        // Whatever 'splitStatements' did not take, (comments, empty ifs) is not needed any more
        எனில் (Ast_முனை* மாறிலி restp = nodep->stmtsp()) {
            VL_DO_DANGLING(restp->unlinkFrBackWithNext()->delete_மரம்(), restp);
        }

        // Every color has a statement in it. Reuse the original block for the
        // first color, and add a new block after it for each of the rest.
        UASSERT_OBJ(lists.front(), nodep, "Color with no statements");
        nodep->addStmtsp(lists.front());
        Ast_முனை* lastp = nodep;
        File_வரி* மாறிலி flp = nodep->fileline();
        மாறிலி VAlwaysKwd kwd = nodep->keyword();
        க்கு (size_t i = 1; i < இலக்கம்_Colors; ++i) {
            Ast_முனை* மாறிலி stmtsp = lists[i];
            UASSERT_OBJ(stmtsp, nodep, "Color with no statements");
            AstAlways* மாறிலி newp = புதிய AstAlways{flp, kwd, nullptr, stmtsp};
            newp->user3(1);  // Do not split again
            lastp->addNextHere(newp);
            lastp = newp;
        }
    }

    வெறுமை visit(AstIf* nodep) override {
        எனில் (!m_graphp || m_noSplitWhy) திரும்பு;
        {
            VL_RESTORER(m_currIfp);
            m_currIfp = nodep;
            iterateAndNextNull(nodep->condp());
        }
        scanBlock(nodep->thensp());
        scanBlock(nodep->elsesp());
    }

    வெறுமை visit(Ast_கோவை_Stmt* nodep) override {
        எனில் (!m_graphp || m_noSplitWhy) திரும்பு;
        VL_RESTORER(m_inDly);
        m_inDly = பொய்;
        iterateChildren(nodep);
    }

    வெறுமை visit(AstAssignDly* nodep) override {
        எனில் (!m_graphp || m_noSplitWhy) திரும்பு;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        எனில் (Ast_முனை* மாறிலி controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_inDly);
        m_inDly = உண்மை;
        iterate(nodep->lhsp());
    }

    வெறுமை visit(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        எனில் (m_graphp) m_noSplitWhy = "Handle";
        visit(static_cast<Ast_முனை*>(nodep));
    }

    வெறுமை visit(AstJumpGo*) override {
        எனில் (!m_graphp || m_noSplitWhy) திரும்பு;
        m_noSplitWhy = "JumpGo";
    }

    வெறுமை visit(AstVarRef* nodep) override {
        எனில் (!m_graphp || m_noSplitWhy) திரும்பு;
        UASSERT_OBJ(!m_stmtStackps.empty(), nodep, "Not under a statement");

        // Constant lookups can be ignored
        எனில் (nodep->varp()->isConst()) திரும்பு;

        AstVarScope* மாறிலி vscp = nodep->varScopep();

        // SPEEDUP: We add duplicate edges, that should be fixed
        எனில் (m_inDly && nodep->access().isWriteOrRW()) {
            // Delayed variable: is different from non-delayed variable, writes to it
            // are not observable while executing this block (NBA not yet committed),
            // so add only a write edge to a separate 'post' vertex.
            எனில் (!vscp->user2p()) vscp->user2p(புதிய SplitVarPostVertex{m_graphp, vscp});
            SplitVarPostVertex* மாறிலி vpostp = vscp->user2u().to<SplitVarPostVertex*>();
            க்கு (SplitStmtVertex* மாறிலி vtxp : m_stmtStackps) addEdge(vpostp, vtxp);
        } இல்லையெனில் எனில் (nodep->access().isWriteOrRW()) {
            // Regular (non-blocking) write: Need to maintain program-flow order
            எனில் (!vscp->user1p()) vscp->user1p(புதிய SplitVarStdVertex{m_graphp, vscp});
            SplitVarStdVertex* மாறிலி vstdp = vscp->user1u().to<SplitVarStdVertex*>();
            க்கு (SplitStmtVertex* மாறிலி vtxp : m_stmtStackps) addEdge(vstdp, vtxp);
        } இல்லையெனில் {
            // Regular (non-blocking) read: Need to maintain program-flow order
            எனில் (!vscp->user1p()) vscp->user1p(புதிய SplitVarStdVertex{m_graphp, vscp});
            SplitVarStdVertex* மாறிலி vstdp = vscp->user1u().to<SplitVarStdVertex*>();
            க்கு (SplitStmtVertex* மாறிலி vtxp : m_stmtStackps) {
                // If this is an if statement it only depends on refs in its
                // own condition only (not those in its branches). For other
                // statements, just record the referene as normal.
                எனில் (மாறிலி AstIf* மாறிலி ifp = VN_CAST(vtxp->nodep(), If)) {
                    எனில் (ifp != m_currIfp) தொடர்;
                }
                addEdge(vtxp, vstdp);
            }
        }
    }

    வெறுமை visit(Ast_முனை* nodep) override {
        // Outside AstAlways, just descend
        எனில் (!m_graphp) {
            iterateChildren(nodep);
            திரும்பு;
        }
        // Early exit if decided not to split
        எனில் (m_noSplitWhy) திரும்பு;

        UASSERT_OBJ(!m_stmtStackps.empty(), nodep, "Not under a statement");

        // Timing control prevents splitting
        எனில் (nodep->isTimingControl()) {
            m_noSplitWhy = "TimingControl";
            திரும்பு;
        }

        // All impure statements must be grouped together.
        எனில் (!nodep->isPure()) {
            எனில் (!m_impureVtxp) m_impureVtxp = புதிய SplitImpureVertex{m_graphp, nodep};
            // One edge is enough to find the weakly connected components, but
            // it must point at the impure vertex, so it is an out edge (input
            // dependency) of any enclosing 'if' to prevent pruning.
            க்கு (SplitStmtVertex* மாறிலி vtxp : m_stmtStackps) addEdge(vtxp, m_impureVtxp);
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
    explicit SplitVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SplitVisitor() override { V3Stats::addStat("Optimizations, Split always", m_statSplits); }
    VL_UNCOPYABLE(SplitVisitor);

பொது:
    நிலையான வெறுமை apply(AstNetlist* nodep) { SplitVisitor{nodep}; }
};

//######################################################################
// Split class functions

வெறுமை V3Split::splitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    SplitVisitor::apply(nodep);
    V3Global::dump_Check_Global_மரம்("split", 0, dump_மரம்_Either_Level() >= 3);
}
