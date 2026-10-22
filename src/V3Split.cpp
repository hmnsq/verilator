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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Split.h"

#شامل "V3Graph.h"
#شامل "V3Stats.h"

#شامل <string>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Support classes

رده Split_گره_Vertex VL_NOT_FINAL : عمومی V3GraphVertex {
    VL_RTTI_IMPL(Split_گره_Vertex, V3GraphVertex)
    Ast_گره* ثابت m_nodep;

محافظتشده:
    Split_گره_Vertex(V3Graph* graphp, Ast_گره* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    // ACCESSORS
    std::string name() ثابت override {
        بازگشت cvtToHex(m_nodep) + ' ' + m_nodep->prettyTypeName();
    }

عمومی:
    Ast_گره* nodep() ثابت { بازگشت m_nodep; }
};

رده SplitImpureVertex final : عمومی Split_گره_Vertex {
    VL_RTTI_IMPL(SplitImpureVertex, Split_گره_Vertex)

    std::string name() ثابت override { بازگشت "*IMPURE*"; }
    std::string dotColor() ثابت override { بازگشت "green"; }

عمومی:
    explicit SplitImpureVertex(V3Graph* graphp, Ast_گره* nodep)
        : Split_گره_Vertex{graphp, nodep} {}
};

رده SplitStmtVertex final : عمومی Split_گره_Vertex {
    VL_RTTI_IMPL(SplitStmtVertex, Split_گره_Vertex)

    std::string dotColor() ثابت override { بازگشت "yellow"; }

عمومی:
    SplitStmtVertex(V3Graph* graphp, Ast_گره* nodep)
        : Split_گره_Vertex{graphp, nodep} {}
};

رده SplitVarStdVertex final : عمومی Split_گره_Vertex {
    VL_RTTI_IMPL(SplitVarStdVertex, Split_گره_Vertex)

    std::string dotColor() ثابت override { بازگشت "skyblue"; }

عمومی:
    SplitVarStdVertex(V3Graph* graphp, AstVarScope* vscp)
        : Split_گره_Vertex{graphp, vscp} {}
};

رده SplitVarPostVertex final : عمومی Split_گره_Vertex {
    VL_RTTI_IMPL(SplitVarPostVertex, Split_گره_Vertex)

    std::string name() ثابت override { بازگشت "POST "s + Split_گره_Vertex::name(); }
    std::string dotColor() ثابت override { بازگشت "CadetBlue"; }

عمومی:
    SplitVarPostVertex(V3Graph* graphp, AstVarScope* vscp)
        : Split_گره_Vertex{graphp, vscp} {}
};

رده SplitVisitor final : عمومی VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> SplitVarStdVertex*: Regular program-flow variable vertex
    // AstVarScope::user2p  -> SplitVarPostVertex*: NBA written delayed variable vertex
    // Ast{StmtIsh}::user1p -> SplitStmtVertex*

    // NODE STATE
    // AstAlways::user3     -> bool: Block created by splitting, needs no further splitting
    ثابت VNUser3InUse m_inuser3;

    // STATE
    V3Graph* m_graphp = nullptr;  // Dependency graph to analyze statement connectivity
    std::vector<SplitStmtVertex*> m_stmtStackps;  // Current statements being tracked
    SplitImpureVertex* m_impureVtxp = nullptr;  // Vertex connecting impure statements
    ثابت نویسه* m_noSplitWhy = nullptr;  // Reason current block cannot be split
    منطقی m_inDly = نادرست;  // Inside AstAssignDly Lhs
    ثابت AstIf* m_currIfp = nullptr;  // The AstIf whose condition is currently visited
    VDouble0 m_statSplits;  // Statistic tracking

    // METHODS
    تهی addEdge(V3GraphVertex* fromp, V3GraphVertex* بالاترین) {
        جدید V3GraphEdge{m_graphp, fromp, بالاترین, 1};
    }

    // Iterate the given list of statements, building the dependency graph
    تهی scanBlock(Ast_گره* stmtsp) {
        اگر (m_noSplitWhy) بازگشت;
        برای (Ast_گره* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Skip comments. They have no dependencies at all, so would always
            // form an independent component, and hence a split block, of their
            // own, which would be subsequently deleted as it does nothing.
            اگر (VN_IS(stmtp, Comment)) ادامه;
            UASSERT_OBJ(!stmtp->user1p(), stmtp, "user1p should not be set");
            SplitStmtVertex* ثابت vtxp = جدید SplitStmtVertex{m_graphp, stmtp};
            stmtp->user1p(vtxp);
            m_stmtStackps.افزودن_back(vtxp);
            iterate(stmtp);
            m_stmtStackps.برداشت_back();
        }
    }

    // Remove unnecessary edges, then color to find weakly connected components
    uint32_t colorAlwaysGraph() {
        اگر (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_built", نادرست);

        // Prune duplicate edges. Not necessary for correctness, but simplifies dumps.
        m_graphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        اگر (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nodup", نادرست);

        // Remove variable vertices that are not written in the block.
        // These are input-only to the block, so carry no dependency.
        برای (V3GraphVertex* ثابت vtxp : m_graphp->vertices().unlinkable()) {
            SplitVarStdVertex* ثابت vstdp = vtxp->cast<SplitVarStdVertex>();
            اگر (!vstdp || !vstdp->outEmpty()) ادامه;
            UINFOTREE(9, vstdp->nodep(), "", "Will remove deps on block input var:");
            vstdp->nodep()->user1p(nullptr);  // Don't leave a dangling pointer behind
            VL_DO_DANGLING(vstdp->unlinkDelete(m_graphp), vstdp);
        }
        اگر (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_noinputs", نادرست);

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
        برای (V3GraphVertex* ثابت vtxp : m_graphp->vertices().unlinkable()) {
            SplitStmtVertex* ثابت stmtVtxp = vtxp->cast<SplitStmtVertex>();
            اگر (!stmtVtxp || !VN_IS(stmtVtxp->nodep(), If)) ادامه;
            // Can't remove if dependent on a variable written in the block
            اگر (!stmtVtxp->outEmpty()) ادامه;
            // Depends only on block inputs, so can be split. Remove the vertex.
            stmtVtxp->nodep()->user1p(nullptr);
            stmtVtxp->unlinkDelete(m_graphp);
        }
        اگر (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nofreeifs", نادرست);

        // Weak coloring to determine what must stay together in a single block
        ثابت uint32_t رقم_Colors = m_graphp->weaklyConnected(&V3GraphEdge::followAlwaysTrue);
        اگر (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_colored", نادرست);
        بازگشت رقم_Colors;
    }

    // Take the statements of the given list, and return them distributed into one list per color.
    ایستا std::vector<Ast_گره*> splitStatements(Ast_گره* stmtsp, uint32_t رقم_Colors) {
        std::vector<Ast_گره*> result{رقم_Colors, nullptr};
        برای (Ast_گره *stmtp = stmtsp, *nextp = nullptr; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();  // 'stmtp' is unlinked below

            // Comments are dropped if the block is split
            اگر (VN_IS(stmtp, Comment)) ادامه;

            // Pick up the statement vertex, which might be nullptr after pruning during analysis
            ثابت SplitStmtVertex* ثابت vtxp = stmtp->user1u().to<SplitStmtVertex*>();

            // If statements are duplicated for each split branch
            اگر (AstIf* ثابت ifp = VN_CAST(stmtp, If)) {
                ثابت auto thens = splitStatements(ifp->thensp(), رقم_Colors);
                ثابت auto elses = splitStatements(ifp->elsesp(), رقم_Colors);
                File_سطر* ثابت flp = ifp->fileline();
                // Rebuild the 'if' for each color present in either branch
                منطقی empty = درست;
                برای (uint32_t color = 0; color < رقم_Colors; ++color) {
                    اگر (!thens[color] && !elses[color]) ادامه;
                    empty = نادرست;
                    // The condition is cloned for each color. An impure condition
                    // keeps the 'if' and all it holds in one component, so just once.
                    AstIf* ثابت clonep = جدید AstIf{flp, ifp->condp()->clone_درخت(درست),
                                                    thens[color], elses[color]};
                    // Preserve pragmas from unique if's so assertions work properly
                    clonep->uniquePragma(ifp->uniquePragma());
                    clonep->unique0Pragma(ifp->unique0Pragma());
                    clonep->priorityPragma(ifp->priorityPragma());
                    result[color] = Ast_گره::addNext(result[color], clonep);
                }
                // There is nothing under the 'if' to guard. If its vertex was
                // removed as having no dependencies at all, then its
                // condition reads only block inputs and is pure, so the whole
                // 'if' can go. Otherwise the condition might have a side
                // effect, so keep just the condition, evaluated as a
                // statement, under the color of the 'if' itself.
                اگر (empty && vtxp) {
                    ثابت uint32_t color = vtxp->color();
                    Ast_گره_عبارت* ثابت condp = ifp->condp();
                    condp->unlinkFrBack();
                    result[color] = Ast_گره::addNext(result[color], جدید Ast_Stmt_عبارت{flp, condp});
                }
                ادامه;
            }

            // Move the leaf into its color's list
            ثابت uint32_t color = vtxp->color();
            result[color] = Ast_گره::addNext(result[color], stmtp->unlinkFrBack());
        }
        بازگشت result;
    }

    // VISITORS
    تهی visit(AstAlways* nodep) override {
        // Skip blocks created below
        اگر (nodep->user3()) بازگشت;

        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_noSplitWhy);
        VL_RESTORER(m_inDly);
        V3Graph graph;
        m_graphp = &graph;
        m_impureVtxp = nullptr;
        m_noSplitWhy = nullptr;
        m_inDly = نادرست;
        UASSERT_OBJ(m_stmtStackps.empty(), nodep, "Statement stack not empty");

        // Build the graph
        ثابت VNUser1InUse user1InUse;
        ثابت VNUser2InUse user2InUse;
        scanBlock(nodep->stmtsp());

        // We might have to give up
        اگر (m_noSplitWhy) {
            UINFO(9, "  NoSplitBlock because " << m_noSplitWhy);
            بازگشت;
        }

        // Color the graph to identify separable statements
        ثابت uint32_t رقم_Colors = colorAlwaysGraph();
        // If the whole block is one component (or empty), then nothing to split
        اگر (رقم_Colors <= 1) بازگشت;

        UINFO(6, "  splitting always " << nodep);

        // Count the number of new blocks inserted into the Ast: '1 -> n' split, so 'n - 1' extra
        m_statSplits += رقم_Colors - 1;

        // Unpick the statements out of the original block, into one list per color
        ثابت auto lists = splitStatements(nodep->stmtsp(), رقم_Colors);
        UASSERT_OBJ(lists.size() == رقم_Colors, nodep, "Inconsistent split");

        // Whatever 'splitStatements' did not take, (comments, empty ifs) is not needed any more
        اگر (Ast_گره* ثابت restp = nodep->stmtsp()) {
            VL_DO_DANGLING(restp->unlinkFrBackWithNext()->delete_درخت(), restp);
        }

        // Every color has a statement in it. Reuse the original block for the
        // first color, and add a new block after it for each of the rest.
        UASSERT_OBJ(lists.front(), nodep, "Color with no statements");
        nodep->addStmtsp(lists.front());
        Ast_گره* lastp = nodep;
        File_سطر* ثابت flp = nodep->fileline();
        ثابت VAlwaysKwd kwd = nodep->keyword();
        برای (size_t i = 1; i < رقم_Colors; ++i) {
            Ast_گره* ثابت stmtsp = lists[i];
            UASSERT_OBJ(stmtsp, nodep, "Color with no statements");
            AstAlways* ثابت newp = جدید AstAlways{flp, kwd, nullptr, stmtsp};
            newp->user3(1);  // Do not split again
            lastp->addNextHere(newp);
            lastp = newp;
        }
    }

    تهی visit(AstIf* nodep) override {
        اگر (!m_graphp || m_noSplitWhy) بازگشت;
        {
            VL_RESTORER(m_currIfp);
            m_currIfp = nodep;
            iterateAndNextNull(nodep->condp());
        }
        scanBlock(nodep->thensp());
        scanBlock(nodep->elsesp());
    }

    تهی visit(Ast_عبارت_Stmt* nodep) override {
        اگر (!m_graphp || m_noSplitWhy) بازگشت;
        VL_RESTORER(m_inDly);
        m_inDly = نادرست;
        iterateChildren(nodep);
    }

    تهی visit(AstAssignDly* nodep) override {
        اگر (!m_graphp || m_noSplitWhy) بازگشت;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        اگر (Ast_گره* ثابت controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_inDly);
        m_inDly = درست;
        iterate(nodep->lhsp());
    }

    تهی visit(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        اگر (m_graphp) m_noSplitWhy = "Handle";
        visit(static_cast<Ast_گره*>(nodep));
    }

    تهی visit(AstJumpGo*) override {
        اگر (!m_graphp || m_noSplitWhy) بازگشت;
        m_noSplitWhy = "JumpGo";
    }

    تهی visit(AstVarRef* nodep) override {
        اگر (!m_graphp || m_noSplitWhy) بازگشت;
        UASSERT_OBJ(!m_stmtStackps.empty(), nodep, "Not under a statement");

        // Constant lookups can be ignored
        اگر (nodep->varp()->isConst()) بازگشت;

        AstVarScope* ثابت vscp = nodep->varScopep();

        // SPEEDUP: We add duplicate edges, that should be fixed
        اگر (m_inDly && nodep->access().isWriteOrRW()) {
            // Delayed variable: is different from non-delayed variable, writes to it
            // are not observable while executing this block (NBA not yet committed),
            // so add only a write edge to a separate 'post' vertex.
            اگر (!vscp->user2p()) vscp->user2p(جدید SplitVarPostVertex{m_graphp, vscp});
            SplitVarPostVertex* ثابت vpostp = vscp->user2u().to<SplitVarPostVertex*>();
            برای (SplitStmtVertex* ثابت vtxp : m_stmtStackps) addEdge(vpostp, vtxp);
        } وگرنه اگر (nodep->access().isWriteOrRW()) {
            // Regular (non-blocking) write: Need to maintain program-flow order
            اگر (!vscp->user1p()) vscp->user1p(جدید SplitVarStdVertex{m_graphp, vscp});
            SplitVarStdVertex* ثابت vstdp = vscp->user1u().to<SplitVarStdVertex*>();
            برای (SplitStmtVertex* ثابت vtxp : m_stmtStackps) addEdge(vstdp, vtxp);
        } وگرنه {
            // Regular (non-blocking) read: Need to maintain program-flow order
            اگر (!vscp->user1p()) vscp->user1p(جدید SplitVarStdVertex{m_graphp, vscp});
            SplitVarStdVertex* ثابت vstdp = vscp->user1u().to<SplitVarStdVertex*>();
            برای (SplitStmtVertex* ثابت vtxp : m_stmtStackps) {
                // If this is an if statement it only depends on refs in its
                // own condition only (not those in its branches). For other
                // statements, just record the referene as normal.
                اگر (ثابت AstIf* ثابت ifp = VN_CAST(vtxp->nodep(), If)) {
                    اگر (ifp != m_currIfp) ادامه;
                }
                addEdge(vtxp, vstdp);
            }
        }
    }

    تهی visit(Ast_گره* nodep) override {
        // Outside AstAlways, just descend
        اگر (!m_graphp) {
            iterateChildren(nodep);
            بازگشت;
        }
        // Early exit if decided not to split
        اگر (m_noSplitWhy) بازگشت;

        UASSERT_OBJ(!m_stmtStackps.empty(), nodep, "Not under a statement");

        // Timing control prevents splitting
        اگر (nodep->isTimingControl()) {
            m_noSplitWhy = "TimingControl";
            بازگشت;
        }

        // All impure statements must be grouped together.
        اگر (!nodep->isPure()) {
            اگر (!m_impureVtxp) m_impureVtxp = جدید SplitImpureVertex{m_graphp, nodep};
            // One edge is enough to find the weakly connected components, but
            // it must point at the impure vertex, so it is an out edge (input
            // dependency) of any enclosing 'if' to prevent pruning.
            برای (SplitStmtVertex* ثابت vtxp : m_stmtStackps) addEdge(vtxp, m_impureVtxp);
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
    explicit SplitVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SplitVisitor() override { V3Stats::addStat("Optimizations, Split always", m_statSplits); }
    VL_UNCOPYABLE(SplitVisitor);

عمومی:
    ایستا تهی apply(AstNetlist* nodep) { SplitVisitor{nodep}; }
};

//######################################################################
// Split class functions

تهی V3Split::splitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    SplitVisitor::apply(nodep);
    V3Global::dump_Check_Global_درخت("split", 0, dump_درخت_Either_Level() >= 3);
}
