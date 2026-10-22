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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Split.h"

#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <string>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Support classes

తరగతి విభజించు_కణుపు_శీర్షం VL_NOT_FINAL : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(విభజించు_కణుపు_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
    Ast_కణుపు* స్థిరరాశి m_nodep;

రక్షిత:
    విభజించు_కణుపు_శీర్షం(V3_లేఖాచిత్రం* graphp, Ast_కణుపు* nodep)
        : V3_లేఖాచిత్రం_శీర్షం{graphp}
        , m_nodep{nodep} {}
    // ACCESSORS
    std::string name() స్థిరరాశి override {
        తిరిగి cvt_గమ్యం_Hex(m_nodep) + ' ' + m_nodep->prettyTypeName();
    }

బహిరంగ:
    Ast_కణుపు* nodep() స్థిరరాశి { తిరిగి m_nodep; }
};

తరగతి విభజించు_Impure_శీర్షం final : బహిరంగ విభజించు_కణుపు_శీర్షం {
    VL_RTTI_IMPL(విభజించు_Impure_శీర్షం, విభజించు_కణుపు_శీర్షం)

    std::string name() స్థిరరాశి override { తిరిగి "*IMPURE*"; }
    std::string dotColor() స్థిరరాశి override { తిరిగి "green"; }

బహిరంగ:
    స్పష్టమైన విభజించు_Impure_శీర్షం(V3_లేఖాచిత్రం* graphp, Ast_కణుపు* nodep)
        : విభజించు_కణుపు_శీర్షం{graphp, nodep} {}
};

తరగతి విభజించు_Stmt_శీర్షం final : బహిరంగ విభజించు_కణుపు_శీర్షం {
    VL_RTTI_IMPL(విభజించు_Stmt_శీర్షం, విభజించు_కణుపు_శీర్షం)

    std::string dotColor() స్థిరరాశి override { తిరిగి "yellow"; }

బహిరంగ:
    విభజించు_Stmt_శీర్షం(V3_లేఖాచిత్రం* graphp, Ast_కణుపు* nodep)
        : విభజించు_కణుపు_శీర్షం{graphp, nodep} {}
};

తరగతి విభజించు_Var_Std_శీర్షం final : బహిరంగ విభజించు_కణుపు_శీర్షం {
    VL_RTTI_IMPL(విభజించు_Var_Std_శీర్షం, విభజించు_కణుపు_శీర్షం)

    std::string dotColor() స్థిరరాశి override { తిరిగి "skyblue"; }

బహిరంగ:
    విభజించు_Var_Std_శీర్షం(V3_లేఖాచిత్రం* graphp, AstVarScope* vscp)
        : విభజించు_కణుపు_శీర్షం{graphp, vscp} {}
};

తరగతి విభజించు_Var_Post_శీర్షం final : బహిరంగ విభజించు_కణుపు_శీర్షం {
    VL_RTTI_IMPL(విభజించు_Var_Post_శీర్షం, విభజించు_కణుపు_శీర్షం)

    std::string name() స్థిరరాశి override { తిరిగి "POST "s + విభజించు_కణుపు_శీర్షం::name(); }
    std::string dotColor() స్థిరరాశి override { తిరిగి "CadetBlue"; }

బహిరంగ:
    విభజించు_Var_Post_శీర్షం(V3_లేఖాచిత్రం* graphp, AstVarScope* vscp)
        : విభజించు_కణుపు_శీర్షం{graphp, vscp} {}
};

తరగతి విభజించు_Visitor final : బహిరంగ VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> SplitVarStdVertex*: Regular program-flow variable vertex
    // AstVarScope::user2p  -> SplitVarPostVertex*: NBA written delayed variable vertex
    // Ast{StmtIsh}::user1p -> SplitStmtVertex*

    // NODE STATE
    // AstAlways::user3     -> bool: Block created by splitting, needs no further splitting
    స్థిరరాశి VNUser3_లోపల_Use m_inuser3;

    // STATE
    V3_లేఖాచిత్రం* m_graphp = శూన్యసూచిక;  // Dependency graph to analyze statement connectivity
    std::సదిశ<విభజించు_Stmt_శీర్షం*> m_stmtStackps;  // Current statements being tracked
    విభజించు_Impure_శీర్షం* m_impureVtxp = శూన్యసూచిక;  // Vertex connecting impure statements
    స్థిరరాశి అక్షరం* m_no_విభజించు_Why = శూన్యసూచిక;  // Reason current block cannot be split
    తార్కిక m_లోపల_Dly = అబద్ధం;  // Inside AstAssignDly Lhs
    స్థిరరాశి AstIf* m_currIfp = శూన్యసూచిక;  // The AstIf whose condition is currently visited
    VDouble0 m_statSplits;  // Statistic tracking

    // METHODS
    శూన్య కూడు_అంచు(V3_లేఖాచిత్రం_శీర్షం* fromp, V3_లేఖాచిత్రం_శీర్షం* శిఖరం) {
        కొత్త V3_లేఖాచిత్రం_అంచు{m_graphp, fromp, శిఖరం, 1};
    }

    // Iterate the given list of statements, building the dependency graph
    శూన్య పరిశీలన_ఖండం(Ast_కణుపు* stmtsp) {
        ఒకవేళ (m_no_విభజించు_Why) తిరిగి;
        కొరకు (Ast_కణుపు* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Skip comments. They have no dependencies at all, so would always
            // form an independent component, and hence a split block, of their
            // own, which would be subsequently deleted as it does nothing.
            ఒకవేళ (VN_IS(stmtp, Comment)) కొనసాగు;
            UASSERT_OBJ(!stmtp->user1p(), stmtp, "user1p should not be set");
            విభజించు_Stmt_శీర్షం* స్థిరరాశి vtxp = కొత్త విభజించు_Stmt_శీర్షం{m_graphp, stmtp};
            stmtp->user1p(vtxp);
            m_stmtStackps.నెట్టు_వెనుక(vtxp);
            iterate(stmtp);
            m_stmtStackps.తీయి_వెనుక();
        }
    }

    // Remove unnecessary edges, then color to find weakly connected components
    uint32_t color_Always_లేఖాచిత్రం() {
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_built", అబద్ధం);

        // Prune duplicate edges. Not necessary for correctness, but simplifies dumps.
        m_graphp->remove_Redundant_అంచులు_Max(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nodup", అబద్ధం);

        // Remove variable vertices that are not written in the block.
        // These are input-only to the block, so carry no dependency.
        కొరకు (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp : m_graphp->vertices().unlinkable()) {
            విభజించు_Var_Std_శీర్షం* స్థిరరాశి vstdp = vtxp->cast<విభజించు_Var_Std_శీర్షం>();
            ఒకవేళ (!vstdp || !vstdp->బహిర్గతం_ఖాళీలు()) కొనసాగు;
            UINFOTREE(9, vstdp->nodep(), "", "Will remove deps on block input var:");
            vstdp->nodep()->user1p(శూన్యసూచిక);  // Don't leave a dangling pointer behind
            VL_DO_DANGLING(vstdp->unlinkDelete(m_graphp), vstdp);
        }
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_noinputs", అబద్ధం);

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
        కొరకు (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp : m_graphp->vertices().unlinkable()) {
            విభజించు_Stmt_శీర్షం* స్థిరరాశి stmtVtxp = vtxp->cast<విభజించు_Stmt_శీర్షం>();
            ఒకవేళ (!stmtVtxp || !VN_IS(stmtVtxp->nodep(), If)) కొనసాగు;
            // Can't remove if dependent on a variable written in the block
            ఒకవేళ (!stmtVtxp->బహిర్గతం_ఖాళీలు()) కొనసాగు;
            // Depends only on block inputs, so can be split. Remove the vertex.
            stmtVtxp->nodep()->user1p(శూన్యసూచిక);
            stmtVtxp->unlinkDelete(m_graphp);
        }
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_nofreeifs", అబద్ధం);

        // Weak coloring to determine what must stay together in a single block
        స్థిరరాశి uint32_t అంకె_Colors = m_graphp->weaklyConnected(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 9) m_graphp->dumpDotFilePrefixed("splitg_colored", అబద్ధం);
        తిరిగి అంకె_Colors;
    }

    // Take the statements of the given list, and return them distributed into one list per color.
    స్థిర std::సదిశ<Ast_కణుపు*> విభజించు_Statements(Ast_కణుపు* stmtsp, uint32_t అంకె_Colors) {
        std::సదిశ<Ast_కణుపు*> ఫలితం{అంకె_Colors, శూన్యసూచిక};
        కొరకు (Ast_కణుపు *stmtp = stmtsp, *nextp = శూన్యసూచిక; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();  // 'stmtp' is unlinked below

            // Comments are dropped if the block is split
            ఒకవేళ (VN_IS(stmtp, Comment)) కొనసాగు;

            // Pick up the statement vertex, which might be nullptr after pruning during analysis
            స్థిరరాశి విభజించు_Stmt_శీర్షం* స్థిరరాశి vtxp = stmtp->user1u().గమ్యం<విభజించు_Stmt_శీర్షం*>();

            // If statements are duplicated for each split branch
            ఒకవేళ (AstIf* స్థిరరాశి ifp = VN_CAST(stmtp, If)) {
                స్థిరరాశి స్వయంచాలక thens = విభజించు_Statements(ifp->thensp(), అంకె_Colors);
                స్థిరరాశి స్వయంచాలక elses = విభజించు_Statements(ifp->elsesp(), అంకె_Colors);
                File_పంక్తి* స్థిరరాశి flp = ifp->fileline();
                // Rebuild the 'if' for each color present in either branch
                తార్కిక ఖాళీలు = నిజం;
                కొరకు (uint32_t color = 0; color < అంకె_Colors; ++color) {
                    ఒకవేళ (!thens[color] && !elses[color]) కొనసాగు;
                    ఖాళీలు = అబద్ధం;
                    // The condition is cloned for each color. An impure condition
                    // keeps the 'if' and all it holds in one component, so just once.
                    AstIf* స్థిరరాశి clonep = కొత్త AstIf{flp, ifp->condp()->clone_వృక్షం(నిజం),
                                                    thens[color], elses[color]};
                    // Preserve pragmas from unique if's so assertions work properly
                    clonep->uniquePragma(ifp->uniquePragma());
                    clonep->unique0Pragma(ifp->unique0Pragma());
                    clonep->ప్రాధాన్యత_Pragma(ifp->ప్రాధాన్యత_Pragma());
                    ఫలితం[color] = Ast_కణుపు::కూడు_తదుపరి(ఫలితం[color], clonep);
                }
                // There is nothing under the 'if' to guard. If its vertex was
                // removed as having no dependencies at all, then its
                // condition reads only block inputs and is pure, so the whole
                // 'if' can go. Otherwise the condition might have a side
                // effect, so keep just the condition, evaluated as a
                // statement, under the color of the 'if' itself.
                ఒకవేళ (ఖాళీలు && vtxp) {
                    స్థిరరాశి uint32_t color = vtxp->color();
                    Ast_కణుపు_సమాసం* స్థిరరాశి condp = ifp->condp();
                    condp->unlink_Fr_వెనుక();
                    ఫలితం[color] = Ast_కణుపు::కూడు_తదుపరి(ఫలితం[color], కొత్త Ast_Stmt_సమాసం{flp, condp});
                }
                కొనసాగు;
            }

            // Move the leaf into its color's list
            స్థిరరాశి uint32_t color = vtxp->color();
            ఫలితం[color] = Ast_కణుపు::కూడు_తదుపరి(ఫలితం[color], stmtp->unlink_Fr_వెనుక());
        }
        తిరిగి ఫలితం;
    }

    // VISITORS
    శూన్య సందర్శించు(AstAlways* nodep) override {
        // Skip blocks created below
        ఒకవేళ (nodep->user3()) తిరిగి;

        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_no_విభజించు_Why);
        VL_RESTORER(m_లోపల_Dly);
        V3_లేఖాచిత్రం లేఖాచిత్రం;
        m_graphp = &లేఖాచిత్రం;
        m_impureVtxp = శూన్యసూచిక;
        m_no_విభజించు_Why = శూన్యసూచిక;
        m_లోపల_Dly = అబద్ధం;
        UASSERT_OBJ(m_stmtStackps.ఖాళీలు(), nodep, "Statement stack not empty");

        // Build the graph
        స్థిరరాశి VNUser1_లోపల_Use user1_లోపల_Use;
        స్థిరరాశి VNUser2_లోపల_Use user2_లోపల_Use;
        పరిశీలన_ఖండం(nodep->stmtsp());

        // We might have to give up
        ఒకవేళ (m_no_విభజించు_Why) {
            UINFO(9, "  NoSplitBlock because " << m_no_విభజించు_Why);
            తిరిగి;
        }

        // Color the graph to identify separable statements
        స్థిరరాశి uint32_t అంకె_Colors = color_Always_లేఖాచిత్రం();
        // If the whole block is one component (or empty), then nothing to split
        ఒకవేళ (అంకె_Colors <= 1) తిరిగి;

        UINFO(6, "  splitting always " << nodep);

        // Count the number of new blocks inserted into the Ast: '1 -> n' split, so 'n - 1' extra
        m_statSplits += అంకె_Colors - 1;

        // Unpick the statements out of the original block, into one list per color
        స్థిరరాశి స్వయంచాలక lists = విభజించు_Statements(nodep->stmtsp(), అంకె_Colors);
        UASSERT_OBJ(lists.కొలత() == అంకె_Colors, nodep, "Inconsistent split");

        // Whatever 'splitStatements' did not take, (comments, empty ifs) is not needed any more
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి restp = nodep->stmtsp()) {
            VL_DO_DANGLING(restp->unlink_Fr_వెనుక_With_తదుపరి()->delete_వృక్షం(), restp);
        }

        // Every color has a statement in it. Reuse the original block for the
        // first color, and add a new block after it for each of the rest.
        UASSERT_OBJ(lists.front(), nodep, "Color with no statements");
        nodep->కూడు_Stmtsp(lists.front());
        Ast_కణుపు* lastp = nodep;
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        స్థిరరాశి VAlwaysKwd kwd = nodep->keyword();
        కొరకు (కొలత_t i = 1; i < అంకె_Colors; ++i) {
            Ast_కణుపు* స్థిరరాశి stmtsp = lists[i];
            UASSERT_OBJ(stmtsp, nodep, "Color with no statements");
            AstAlways* స్థిరరాశి newp = కొత్త AstAlways{flp, kwd, శూన్యసూచిక, stmtsp};
            newp->user3(1);  // Do not split again
            lastp->కూడు_తదుపరి_ఇక్కడ(newp);
            lastp = newp;
        }
    }

    శూన్య సందర్శించు(AstIf* nodep) override {
        ఒకవేళ (!m_graphp || m_no_విభజించు_Why) తిరిగి;
        {
            VL_RESTORER(m_currIfp);
            m_currIfp = nodep;
            iterate_And_తదుపరి_Null(nodep->condp());
        }
        పరిశీలన_ఖండం(nodep->thensp());
        పరిశీలన_ఖండం(nodep->elsesp());
    }

    శూన్య సందర్శించు(Ast_సమాసం_Stmt* nodep) override {
        ఒకవేళ (!m_graphp || m_no_విభజించు_Why) తిరిగి;
        VL_RESTORER(m_లోపల_Dly);
        m_లోపల_Dly = అబద్ధం;
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(Ast_కేటాయించు_Dly* nodep) override {
        ఒకవేళ (!m_graphp || m_no_విభజించు_Why) తిరిగి;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_లోపల_Dly);
        m_లోపల_Dly = నిజం;
        iterate(nodep->lhsp());
    }

    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        ఒకవేళ (m_graphp) m_no_విభజించు_Why = "Handle";
        సందర్శించు(static_cast<Ast_కణుపు*>(nodep));
    }

    శూన్య సందర్శించు(AstJumpGo*) override {
        ఒకవేళ (!m_graphp || m_no_విభజించు_Why) తిరిగి;
        m_no_విభజించు_Why = "JumpGo";
    }

    శూన్య సందర్శించు(AstVarRef* nodep) override {
        ఒకవేళ (!m_graphp || m_no_విభజించు_Why) తిరిగి;
        UASSERT_OBJ(!m_stmtStackps.ఖాళీలు(), nodep, "Not under a statement");

        // Constant lookups can be ignored
        ఒకవేళ (nodep->varp()->isConst()) తిరిగి;

        AstVarScope* స్థిరరాశి vscp = nodep->varScopep();

        // SPEEDUP: We add duplicate edges, that should be fixed
        ఒకవేళ (m_లోపల_Dly && nodep->access().isWriteOrRW()) {
            // Delayed variable: is different from non-delayed variable, writes to it
            // are not observable while executing this block (NBA not yet committed),
            // so add only a write edge to a separate 'post' vertex.
            ఒకవేళ (!vscp->user2p()) vscp->user2p(కొత్త విభజించు_Var_Post_శీర్షం{m_graphp, vscp});
            విభజించు_Var_Post_శీర్షం* స్థిరరాశి vpostp = vscp->user2u().గమ్యం<విభజించు_Var_Post_శీర్షం*>();
            కొరకు (విభజించు_Stmt_శీర్షం* స్థిరరాశి vtxp : m_stmtStackps) కూడు_అంచు(vpostp, vtxp);
        } లేకపోతే ఒకవేళ (nodep->access().isWriteOrRW()) {
            // Regular (non-blocking) write: Need to maintain program-flow order
            ఒకవేళ (!vscp->user1p()) vscp->user1p(కొత్త విభజించు_Var_Std_శీర్షం{m_graphp, vscp});
            విభజించు_Var_Std_శీర్షం* స్థిరరాశి vstdp = vscp->user1u().గమ్యం<విభజించు_Var_Std_శీర్షం*>();
            కొరకు (విభజించు_Stmt_శీర్షం* స్థిరరాశి vtxp : m_stmtStackps) కూడు_అంచు(vstdp, vtxp);
        } లేకపోతే {
            // Regular (non-blocking) read: Need to maintain program-flow order
            ఒకవేళ (!vscp->user1p()) vscp->user1p(కొత్త విభజించు_Var_Std_శీర్షం{m_graphp, vscp});
            విభజించు_Var_Std_శీర్షం* స్థిరరాశి vstdp = vscp->user1u().గమ్యం<విభజించు_Var_Std_శీర్షం*>();
            కొరకు (విభజించు_Stmt_శీర్షం* స్థిరరాశి vtxp : m_stmtStackps) {
                // If this is an if statement it only depends on refs in its
                // own condition only (not those in its branches). For other
                // statements, just record the referene as normal.
                ఒకవేళ (స్థిరరాశి AstIf* స్థిరరాశి ifp = VN_CAST(vtxp->nodep(), If)) {
                    ఒకవేళ (ifp != m_currIfp) కొనసాగు;
                }
                కూడు_అంచు(vtxp, vstdp);
            }
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        // Outside AstAlways, just descend
        ఒకవేళ (!m_graphp) {
            iterateChildren(nodep);
            తిరిగి;
        }
        // Early exit if decided not to split
        ఒకవేళ (m_no_విభజించు_Why) తిరిగి;

        UASSERT_OBJ(!m_stmtStackps.ఖాళీలు(), nodep, "Not under a statement");

        // Timing control prevents splitting
        ఒకవేళ (nodep->isTimingControl()) {
            m_no_విభజించు_Why = "TimingControl";
            తిరిగి;
        }

        // All impure statements must be grouped together.
        ఒకవేళ (!nodep->is_శుద్ధ()) {
            ఒకవేళ (!m_impureVtxp) m_impureVtxp = కొత్త విభజించు_Impure_శీర్షం{m_graphp, nodep};
            // One edge is enough to find the weakly connected components, but
            // it must point at the impure vertex, so it is an out edge (input
            // dependency) of any enclosing 'if' to prevent pruning.
            కొరకు (విభజించు_Stmt_శీర్షం* స్థిరరాశి vtxp : m_stmtStackps) కూడు_అంచు(vtxp, m_impureVtxp);
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
    స్పష్టమైన విభజించు_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~విభజించు_Visitor() override { V3Stats::కూడు_Stat("Optimizations, Split always", m_statSplits); }
    VL_UNCOPYABLE(విభజించు_Visitor);

బహిరంగ:
    స్థిర శూన్య apply(AstNetlist* nodep) { విభజించు_Visitor{nodep}; }
};

//######################################################################
// Split class functions

శూన్య V3_విభజించు::విభజించు_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    విభజించు_Visitor::apply(nodep);
    V3Global::dump_Check_Global_వృక్షం("split", 0, dump_వృక్షం_Either_Level() >= 3);
}
