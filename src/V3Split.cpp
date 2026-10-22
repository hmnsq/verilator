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

वर्ग विभज_ग्रन्थिः_Vertex VL_NOT_FINAL : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(विभज_ग्रन्थिः_Vertex, V3GraphVertex)
    Ast_ग्रन्थिः* ध्रुव m_nodep;

रक्षित:
    विभज_ग्रन्थिः_Vertex(V3Graph* graphp, Ast_ग्रन्थिः* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    // ACCESSORS
    std::string name() ध्रुव override {
        प्रत्यागम cvt_गन्तव्यम्_Hex(m_nodep) + ' ' + m_nodep->prettyTypeName();
    }

सार्वजनिक:
    Ast_ग्रन्थिः* nodep() ध्रुव { प्रत्यागम m_nodep; }
};

वर्ग विभज_Impure_Vertex final : सार्वजनिक विभज_ग्रन्थिः_Vertex {
    VL_RTTI_IMPL(विभज_Impure_Vertex, विभज_ग्रन्थिः_Vertex)

    std::string name() ध्रुव override { प्रत्यागम "*IMPURE*"; }
    std::string dotColor() ध्रुव override { प्रत्यागम "green"; }

सार्वजनिक:
    स्पष्ट विभज_Impure_Vertex(V3Graph* graphp, Ast_ग्रन्थिः* nodep)
        : विभज_ग्रन्थिः_Vertex{graphp, nodep} {}
};

वर्ग विभज_Stmt_Vertex final : सार्वजनिक विभज_ग्रन्थिः_Vertex {
    VL_RTTI_IMPL(विभज_Stmt_Vertex, विभज_ग्रन्थिः_Vertex)

    std::string dotColor() ध्रुव override { प्रत्यागम "yellow"; }

सार्वजनिक:
    विभज_Stmt_Vertex(V3Graph* graphp, Ast_ग्रन्थिः* nodep)
        : विभज_ग्रन्थिः_Vertex{graphp, nodep} {}
};

वर्ग विभज_Var_Std_Vertex final : सार्वजनिक विभज_ग्रन्थिः_Vertex {
    VL_RTTI_IMPL(विभज_Var_Std_Vertex, विभज_ग्रन्थिः_Vertex)

    std::string dotColor() ध्रुव override { प्रत्यागम "skyblue"; }

सार्वजनिक:
    विभज_Var_Std_Vertex(V3Graph* graphp, AstVarScope* vscp)
        : विभज_ग्रन्थिः_Vertex{graphp, vscp} {}
};

वर्ग विभज_Var_Post_Vertex final : सार्वजनिक विभज_ग्रन्थिः_Vertex {
    VL_RTTI_IMPL(विभज_Var_Post_Vertex, विभज_ग्रन्थिः_Vertex)

    std::string name() ध्रुव override { प्रत्यागम "POST "s + विभज_ग्रन्थिः_Vertex::name(); }
    std::string dotColor() ध्रुव override { प्रत्यागम "CadetBlue"; }

सार्वजनिक:
    विभज_Var_Post_Vertex(V3Graph* graphp, AstVarScope* vscp)
        : विभज_ग्रन्थिः_Vertex{graphp, vscp} {}
};

वर्ग विभज_Visitor final : सार्वजनिक VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> SplitVarStdVertex*: Regular program-flow variable vertex
    // AstVarScope::user2p  -> SplitVarPostVertex*: NBA written delayed variable vertex
    // Ast{StmtIsh}::user1p -> SplitStmtVertex*

    // NODE STATE
    // AstAlways::user3     -> bool: Block created by splitting, needs no further splitting
    ध्रुव VNUser3_अन्तर्_Use m_inuser3;

    // STATE
    V3Graph* m_graphp = शून्यसूचकः;  // Dependency graph to analyze statement connectivity
    std::सदिशम्<विभज_Stmt_Vertex*> m_stmtStackps;  // Current statements being tracked
    विभज_Impure_Vertex* m_impureVtxp = शून्यसूचकः;  // Vertex connecting impure statements
    ध्रुव वर्ण* m_no_विभज_Why = शून्यसूचकः;  // Reason current block cannot be split
    तार्किक m_अन्तर्_Dly = असत्य;  // Inside AstAssignDly Lhs
    ध्रुव AstIf* m_currIfp = शून्यसूचकः;  // The AstIf whose condition is currently visited
    VDouble0 m_statSplits;  // Statistic tracking

    // METHODS
    शून्य सङ्कलय_Edge(V3GraphVertex* fromp, V3GraphVertex* शिखरम्) {
        नूतन V3GraphEdge{m_graphp, fromp, शिखरम्, 1};
    }

    // Iterate the given list of statements, building the dependency graph
    शून्य अवलोकनम्_खण्डम्(Ast_ग्रन्थिः* stmtsp) {
        यदि (m_no_विभज_Why) प्रत्यागम;
        प्रति (Ast_ग्रन्थिः* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Skip comments. They have no dependencies at all, so would always
            // form an independent component, and hence a split block, of their
            // own, which would be subsequently deleted as it does nothing.
            यदि (VN_IS(stmtp, Comment)) अनुवर्त;
            UASSERT_OBJ(!stmtp->user1p(), stmtp, "user1p should not be set");
            विभज_Stmt_Vertex* ध्रुव vtxp = नूतन विभज_Stmt_Vertex{m_graphp, stmtp};
            stmtp->user1p(vtxp);
            m_stmtStackps.प्रक्षिप_पृष्ठभागः(vtxp);
            iterate(stmtp);
            m_stmtStackps.निष्कासय_पृष्ठभागः();
        }
    }

    // Remove unnecessary edges, then color to find weakly connected components
    uint32_t colorAlwaysGraph() {
        यदि (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_built", असत्य);

        // Prune duplicate edges. Not necessary for correctness, but simplifies dumps.
        m_graphp->remove_Redundant_धाराः_Max(&V3GraphEdge::followAlwaysTrue);
        यदि (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nodup", असत्य);

        // Remove variable vertices that are not written in the block.
        // These are input-only to the block, so carry no dependency.
        प्रति (V3GraphVertex* ध्रुव vtxp : m_graphp->vertices().unlinkable()) {
            विभज_Var_Std_Vertex* ध्रुव vstdp = vtxp->cast<विभज_Var_Std_Vertex>();
            यदि (!vstdp || !vstdp->बहिर्गमनम्_रिक्तानि()) अनुवर्त;
            UINFOTREE(9, vstdp->nodep(), "", "Will remove deps on block input var:");
            vstdp->nodep()->user1p(शून्यसूचकः);  // Don't leave a dangling pointer behind
            VL_DO_DANGLING(vstdp->unlinkDelete(m_graphp), vstdp);
        }
        यदि (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_noinputs", असत्य);

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
        प्रति (V3GraphVertex* ध्रुव vtxp : m_graphp->vertices().unlinkable()) {
            विभज_Stmt_Vertex* ध्रुव stmtVtxp = vtxp->cast<विभज_Stmt_Vertex>();
            यदि (!stmtVtxp || !VN_IS(stmtVtxp->nodep(), If)) अनुवर्त;
            // Can't remove if dependent on a variable written in the block
            यदि (!stmtVtxp->बहिर्गमनम्_रिक्तानि()) अनुवर्त;
            // Depends only on block inputs, so can be split. Remove the vertex.
            stmtVtxp->nodep()->user1p(शून्यसूचकः);
            stmtVtxp->unlinkDelete(m_graphp);
        }
        यदि (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nofreeifs", असत्य);

        // Weak coloring to determine what must stay together in a single block
        ध्रुव uint32_t अङ्कः_Colors = m_graphp->weaklyConnected(&V3GraphEdge::followAlwaysTrue);
        यदि (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("splitg_colored", असत्य);
        प्रत्यागम अङ्कः_Colors;
    }

    // Take the statements of the given list, and return them distributed into one list per color.
    स्थिर std::सदिशम्<Ast_ग्रन्थिः*> विभज_Statements(Ast_ग्रन्थिः* stmtsp, uint32_t अङ्कः_Colors) {
        std::सदिशम्<Ast_ग्रन्थिः*> परिणाम{अङ्कः_Colors, शून्यसूचकः};
        प्रति (Ast_ग्रन्थिः *stmtp = stmtsp, *nextp = शून्यसूचकः; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();  // 'stmtp' is unlinked below

            // Comments are dropped if the block is split
            यदि (VN_IS(stmtp, Comment)) अनुवर्त;

            // Pick up the statement vertex, which might be nullptr after pruning during analysis
            ध्रुव विभज_Stmt_Vertex* ध्रुव vtxp = stmtp->user1u().गन्तव्यम्<विभज_Stmt_Vertex*>();

            // If statements are duplicated for each split branch
            यदि (AstIf* ध्रुव ifp = VN_CAST(stmtp, If)) {
                ध्रुव स्वचालित thens = विभज_Statements(ifp->thensp(), अङ्कः_Colors);
                ध्रुव स्वचालित elses = विभज_Statements(ifp->elsesp(), अङ्कः_Colors);
                File_पङ्क्तिः* ध्रुव flp = ifp->fileline();
                // Rebuild the 'if' for each color present in either branch
                तार्किक रिक्तानि = सत्य;
                प्रति (uint32_t color = 0; color < अङ्कः_Colors; ++color) {
                    यदि (!thens[color] && !elses[color]) अनुवर्त;
                    रिक्तानि = असत्य;
                    // The condition is cloned for each color. An impure condition
                    // keeps the 'if' and all it holds in one component, so just once.
                    AstIf* ध्रुव clonep = नूतन AstIf{flp, ifp->condp()->clone_वृक्षः(सत्य),
                                                    thens[color], elses[color]};
                    // Preserve pragmas from unique if's so assertions work properly
                    clonep->uniquePragma(ifp->uniquePragma());
                    clonep->unique0Pragma(ifp->unique0Pragma());
                    clonep->प्राथमिकता_Pragma(ifp->प्राथमिकता_Pragma());
                    परिणाम[color] = Ast_ग्रन्थिः::सङ्कलय_अग्र(परिणाम[color], clonep);
                }
                // There is nothing under the 'if' to guard. If its vertex was
                // removed as having no dependencies at all, then its
                // condition reads only block inputs and is pure, so the whole
                // 'if' can go. Otherwise the condition might have a side
                // effect, so keep just the condition, evaluated as a
                // statement, under the color of the 'if' itself.
                यदि (रिक्तानि && vtxp) {
                    ध्रुव uint32_t color = vtxp->color();
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = ifp->condp();
                    condp->unlink_Fr_पृष्ठभागः();
                    परिणाम[color] = Ast_ग्रन्थिः::सङ्कलय_अग्र(परिणाम[color], नूतन Ast_Stmt_व्यञ्जकम्{flp, condp});
                }
                अनुवर्त;
            }

            // Move the leaf into its color's list
            ध्रुव uint32_t color = vtxp->color();
            परिणाम[color] = Ast_ग्रन्थिः::सङ्कलय_अग्र(परिणाम[color], stmtp->unlink_Fr_पृष्ठभागः());
        }
        प्रत्यागम परिणाम;
    }

    // VISITORS
    शून्य दर्शय(AstAlways* nodep) override {
        // Skip blocks created below
        यदि (nodep->user3()) प्रत्यागम;

        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_no_विभज_Why);
        VL_RESTORER(m_अन्तर्_Dly);
        V3Graph graph;
        m_graphp = &graph;
        m_impureVtxp = शून्यसूचकः;
        m_no_विभज_Why = शून्यसूचकः;
        m_अन्तर्_Dly = असत्य;
        UASSERT_OBJ(m_stmtStackps.रिक्तानि(), nodep, "Statement stack not empty");

        // Build the graph
        ध्रुव VNUser1_अन्तर्_Use user1_अन्तर्_Use;
        ध्रुव VNUser2_अन्तर्_Use user2_अन्तर्_Use;
        अवलोकनम्_खण्डम्(nodep->stmtsp());

        // We might have to give up
        यदि (m_no_विभज_Why) {
            UINFO(9, "  NoSplitBlock because " << m_no_विभज_Why);
            प्रत्यागम;
        }

        // Color the graph to identify separable statements
        ध्रुव uint32_t अङ्कः_Colors = colorAlwaysGraph();
        // If the whole block is one component (or empty), then nothing to split
        यदि (अङ्कः_Colors <= 1) प्रत्यागम;

        UINFO(6, "  splitting always " << nodep);

        // Count the number of new blocks inserted into the Ast: '1 -> n' split, so 'n - 1' extra
        m_statSplits += अङ्कः_Colors - 1;

        // Unpick the statements out of the original block, into one list per color
        ध्रुव स्वचालित lists = विभज_Statements(nodep->stmtsp(), अङ्कः_Colors);
        UASSERT_OBJ(lists.आकार() == अङ्कः_Colors, nodep, "Inconsistent split");

        // Whatever 'splitStatements' did not take, (comments, empty ifs) is not needed any more
        यदि (Ast_ग्रन्थिः* ध्रुव restp = nodep->stmtsp()) {
            VL_DO_DANGLING(restp->unlink_Fr_पृष्ठभागः_With_अग्र()->delete_वृक्षः(), restp);
        }

        // Every color has a statement in it. Reuse the original block for the
        // first color, and add a new block after it for each of the rest.
        UASSERT_OBJ(lists.front(), nodep, "Color with no statements");
        nodep->सङ्कलय_Stmtsp(lists.front());
        Ast_ग्रन्थिः* lastp = nodep;
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        ध्रुव VAlwaysKwd kwd = nodep->keyword();
        प्रति (आकार_t i = 1; i < अङ्कः_Colors; ++i) {
            Ast_ग्रन्थिः* ध्रुव stmtsp = lists[i];
            UASSERT_OBJ(stmtsp, nodep, "Color with no statements");
            AstAlways* ध्रुव newp = नूतन AstAlways{flp, kwd, शून्यसूचकः, stmtsp};
            newp->user3(1);  // Do not split again
            lastp->सङ्कलय_अग्र_अत्र(newp);
            lastp = newp;
        }
    }

    शून्य दर्शय(AstIf* nodep) override {
        यदि (!m_graphp || m_no_विभज_Why) प्रत्यागम;
        {
            VL_RESTORER(m_currIfp);
            m_currIfp = nodep;
            iterate_And_अग्र_Null(nodep->condp());
        }
        अवलोकनम्_खण्डम्(nodep->thensp());
        अवलोकनम्_खण्डम्(nodep->elsesp());
    }

    शून्य दर्शय(Ast_व्यञ्जकम्_Stmt* nodep) override {
        यदि (!m_graphp || m_no_विभज_Why) प्रत्यागम;
        VL_RESTORER(m_अन्तर्_Dly);
        m_अन्तर्_Dly = असत्य;
        iterateChildren(nodep);
    }

    शून्य दर्शय(Ast_नियोजय_Dly* nodep) override {
        यदि (!m_graphp || m_no_विभज_Why) प्रत्यागम;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        यदि (Ast_ग्रन्थिः* ध्रुव controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_अन्तर्_Dly);
        m_अन्तर्_Dly = सत्य;
        iterate(nodep->lhsp());
    }

    शून्य दर्शय(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        यदि (m_graphp) m_no_विभज_Why = "Handle";
        दर्शय(static_cast<Ast_ग्रन्थिः*>(nodep));
    }

    शून्य दर्शय(AstJumpGo*) override {
        यदि (!m_graphp || m_no_विभज_Why) प्रत्यागम;
        m_no_विभज_Why = "JumpGo";
    }

    शून्य दर्शय(AstVarRef* nodep) override {
        यदि (!m_graphp || m_no_विभज_Why) प्रत्यागम;
        UASSERT_OBJ(!m_stmtStackps.रिक्तानि(), nodep, "Not under a statement");

        // Constant lookups can be ignored
        यदि (nodep->varp()->isConst()) प्रत्यागम;

        AstVarScope* ध्रुव vscp = nodep->varScopep();

        // SPEEDUP: We add duplicate edges, that should be fixed
        यदि (m_अन्तर्_Dly && nodep->access().isWriteOrRW()) {
            // Delayed variable: is different from non-delayed variable, writes to it
            // are not observable while executing this block (NBA not yet committed),
            // so add only a write edge to a separate 'post' vertex.
            यदि (!vscp->user2p()) vscp->user2p(नूतन विभज_Var_Post_Vertex{m_graphp, vscp});
            विभज_Var_Post_Vertex* ध्रुव vpostp = vscp->user2u().गन्तव्यम्<विभज_Var_Post_Vertex*>();
            प्रति (विभज_Stmt_Vertex* ध्रुव vtxp : m_stmtStackps) सङ्कलय_Edge(vpostp, vtxp);
        } अन्यथा यदि (nodep->access().isWriteOrRW()) {
            // Regular (non-blocking) write: Need to maintain program-flow order
            यदि (!vscp->user1p()) vscp->user1p(नूतन विभज_Var_Std_Vertex{m_graphp, vscp});
            विभज_Var_Std_Vertex* ध्रुव vstdp = vscp->user1u().गन्तव्यम्<विभज_Var_Std_Vertex*>();
            प्रति (विभज_Stmt_Vertex* ध्रुव vtxp : m_stmtStackps) सङ्कलय_Edge(vstdp, vtxp);
        } अन्यथा {
            // Regular (non-blocking) read: Need to maintain program-flow order
            यदि (!vscp->user1p()) vscp->user1p(नूतन विभज_Var_Std_Vertex{m_graphp, vscp});
            विभज_Var_Std_Vertex* ध्रुव vstdp = vscp->user1u().गन्तव्यम्<विभज_Var_Std_Vertex*>();
            प्रति (विभज_Stmt_Vertex* ध्रुव vtxp : m_stmtStackps) {
                // If this is an if statement it only depends on refs in its
                // own condition only (not those in its branches). For other
                // statements, just record the referene as normal.
                यदि (ध्रुव AstIf* ध्रुव ifp = VN_CAST(vtxp->nodep(), If)) {
                    यदि (ifp != m_currIfp) अनुवर्त;
                }
                सङ्कलय_Edge(vtxp, vstdp);
            }
        }
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        // Outside AstAlways, just descend
        यदि (!m_graphp) {
            iterateChildren(nodep);
            प्रत्यागम;
        }
        // Early exit if decided not to split
        यदि (m_no_विभज_Why) प्रत्यागम;

        UASSERT_OBJ(!m_stmtStackps.रिक्तानि(), nodep, "Not under a statement");

        // Timing control prevents splitting
        यदि (nodep->isTimingControl()) {
            m_no_विभज_Why = "TimingControl";
            प्रत्यागम;
        }

        // All impure statements must be grouped together.
        यदि (!nodep->is_विशुद्धम्()) {
            यदि (!m_impureVtxp) m_impureVtxp = नूतन विभज_Impure_Vertex{m_graphp, nodep};
            // One edge is enough to find the weakly connected components, but
            // it must point at the impure vertex, so it is an out edge (input
            // dependency) of any enclosing 'if' to prevent pruning.
            प्रति (विभज_Stmt_Vertex* ध्रुव vtxp : m_stmtStackps) सङ्कलय_Edge(vtxp, m_impureVtxp);
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
    स्पष्ट विभज_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~विभज_Visitor() override { V3Stats::सङ्कलय_Stat("Optimizations, Split always", m_statSplits); }
    VL_UNCOPYABLE(विभज_Visitor);

सार्वजनिक:
    स्थिर शून्य apply(AstNetlist* nodep) { विभज_Visitor{nodep}; }
};

//######################################################################
// Split class functions

शून्य V3_विभज::विभज_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    विभज_Visitor::apply(nodep);
    V3Global::dump_Check_Global_वृक्षः("split", 0, dump_वृक्षः_Either_Level() >= 3);
}
