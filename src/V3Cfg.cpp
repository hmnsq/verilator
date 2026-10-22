// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Control flow graph (CFG) implementation
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
//
//  Control flow graph (CFG) implementation
//
//*************************************************************************

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Cfg.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3EmitV.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// CfgBlock method definitions

std::string Cfg_ఖండం::name() స్థిరరాశి {
    std::stringstream ss;
    ss << "BB " + std::గమ్యం_string(id()) + ":\n";
    కొరకు (Ast_కణుపు* nodep : m_stmtps) {
        ఒకవేళ (స్థిరరాశి AstIf* స్థిరరాశి ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debug_Verilog_For_వృక్షం(ifp->condp(), ss);
            ss << ") ...";
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstLoopTest* స్థిరరాశి testp = VN_CAST(nodep, LoopTest)) {
            ss << "if  (!";
            V3EmitV::debug_Verilog_For_వృక్షం(testp->condp(), ss);
            ss << ") break;";
        } లేకపోతే {
            V3EmitV::debug_Verilog_For_వృక్షం(nodep, ss);
        }
    }
    std::string పాఠ్యం = VString::replaceSubstr(
        VString::replaceSubstr(ss.str(), "\n", "\\l        "), "\"", "\\\"");
    ఒకవేళ (isEnter()) పాఠ్యం = "**ENTER**\n" + పాఠ్యం;
    ఒకవేళ (isExit()) పాఠ్యం = పాఠ్యం + "\n**EXIT**";
    తిరిగి పాఠ్యం;
}
std::string Cfg_ఖండం::dotShape() స్థిరరాశి { తిరిగి "rect"; }
std::string Cfg_ఖండం::dotRank() స్థిరరాశి {
    ఒకవేళ (isEnter()) తిరిగి "source";
    ఒకవేళ (isExit()) తిరిగి "sink";
    తిరిగి "";
}

//######################################################################
// CfgEdge method definitions

std::string Cfg_అంచు::dotLabel() స్థిరరాశి {
    std::string label = "E" + std::గమ్యం_string(id());
    స్థిరరాశి Cfg_అంచు* స్థిరరాశి untknp = srcp()->untknEdgep();
    ఒకవేళ (ఇది == untknp) {
        label += " / F";
    } లేకపోతే ఒకవేళ (untknp) {
        label += " / T";
    }
    తిరిగి label;
}

//######################################################################
// CfgGraph method definitions

స్థిర శూన్య cfg_క్రమం_సందర్శించు_ఖండం(std::సదిశ<Cfg_ఖండం*>& post_క్రమం_Enumeration, Cfg_ఖండం* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    ఒకవేళ (Cfg_ఖండం* స్థిరరాశి takenp = bbp->takenp()) {
        ఒకవేళ (!takenp->user()) cfg_క్రమం_సందర్శించు_ఖండం(post_క్రమం_Enumeration, takenp);
        ఒకవేళ (Cfg_ఖండం* స్థిరరాశి untknp = bbp->untknp()) {
            ఒకవేళ (!untknp->user()) cfg_క్రమం_సందర్శించు_ఖండం(post_క్రమం_Enumeration, untknp);
        }
    }
    // Add to post order enumeration
    post_క్రమం_Enumeration.emplace_వెనుక(bbp);
};

శూన్య Cfg_లేఖాచిత్రం::rpo_ఖండాలు() {
    UASSERT_OBJ(m_nEdits != m_n_చివరి_Ordered, m_enterp, "Redundant 'CfgGraph::order' call");
    m_n_చివరి_Ordered = m_nEdits;

    // Reset marks
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::సదిశ<Cfg_ఖండం*> post_క్రమం_Enumeration;
    post_క్రమం_Enumeration.reserve(m_n_ఖండాలు);
    cfg_క్రమం_సందర్శించు_ఖండం(post_క్రమం_Enumeration, m_enterp);
    UASSERT_OBJ(post_క్రమం_Enumeration.కొలత() == m_n_ఖండాలు, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    కొరకు (కొలత_t i = 0; i < post_క్రమం_Enumeration.కొలత(); ++i) {
        Cfg_ఖండం* స్థిరరాశి bbp = post_క్రమం_Enumeration[m_n_ఖండాలు - 1 - i];
        bbp->m_rpo_సంఖ్య = i;
        vertices().unlink(bbp);
        vertices().link_వెనుక(bbp);
    }

    // Assign edge IDs
    కొలత_t అంచు_లెక్క = 0;
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        కొరకు (V3_లేఖాచిత్రం_అంచు& e : v.బహిర్గతం_అంచులు()) static_cast<Cfg_అంచు&>(e).m_id = అంచు_లెక్క++;
    }
    UASSERT_OBJ(అంచు_లెక్క == m_n_అంచులు, m_enterp, "Inconsistent edge count");
}

తార్కిక Cfg_లేఖాచిత్రం::కలిగియుంది_Loop() స్థిరరాశి {
    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : vertices()) {
        స్థిరరాశి Cfg_ఖండం& current = static_cast<స్థిరరాశి Cfg_ఖండం&>(vtx);
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : current.బహిర్గతం_అంచులు()) {
            స్థిరరాశి Cfg_ఖండం& successor = *static_cast<స్థిరరాశి Cfg_ఖండం*>(అంచు.శిఖరం());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            ఒకవేళ (successor.id() < current.id()) తిరిగి నిజం;
        }
    }
    తిరిగి అబద్ధం;
}

శూన్య Cfg_లేఖాచిత్రం::minimize() {
    // Remove empty blocks (except enter and exit)
    కొరకు (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp : vertices().unlinkable()) {
        Cfg_ఖండం* స్థిరరాశి bbp = static_cast<Cfg_ఖండం*>(vtxp);
        ఒకవేళ (bbp->isEnter()) కొనసాగు;
        ఒకవేళ (bbp->isExit()) కొనసాగు;
        ఒకవేళ (!bbp->stmtps().ఖాళీలు()) కొనసాగు;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        Cfg_ఖండం* స్థిరరాశి succp = bbp->takenp();
        కొరకు (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep : bbp->లోపల_అంచులు().unlinkable()) edgep->relink_శిఖరం(succp);
        ++m_nEdits;
        --m_n_అంచులు;
        --m_n_ఖండాలు;
        VL_DO_DANGLING(bbp->unlinkDelete(ఇది), bbp);
    }

    // Combine sequential blocks
    కొరకు (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp : vertices().unlinkable()) {
        Cfg_ఖండం* స్థిరరాశి srcp = static_cast<Cfg_ఖండం*>(vtxp);
        ఒకవేళ (srcp->isExit()) కొనసాగు;
        ఒకవేళ (srcp->isBranch()) కొనసాగు;
        Cfg_ఖండం* స్థిరరాశి dstp = srcp->takenp();
        ఒకవేళ (dstp->isJoin()) కొనసాగు;
        // Combine them
        ఒకవేళ (srcp->isEnter()) m_enterp = dstp;
        std::సదిశ<Ast_కణుపు_Stmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.కొలత() + dstp->m_stmtps.కొలత());
        stmtps.చొప్పించు(stmtps.చివర(), dstp->m_stmtps.begin(), dstp->m_stmtps.చివర());
        dstp->m_stmtps = std::move(stmtps);
        కొరకు (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep : srcp->లోపల_అంచులు().unlinkable()) edgep->relink_శిఖరం(dstp);
        ++m_nEdits;
        --m_n_అంచులు;
        --m_n_ఖండాలు;
        VL_DO_DANGLING(srcp->unlinkDelete(ఇది), srcp);
    }

    ఒకవేళ (m_nEdits != m_n_చివరి_Ordered) rpo_ఖండాలు();
    ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

శూన్య Cfg_లేఖాచిత్రం::break_Critical_అంచులు() {
    // Gather critical edges
    std::సదిశ<Cfg_అంచు*> critical_అంచులు;
    critical_అంచులు.reserve(m_n_అంచులు);
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : vertices()) {
        స్థిరరాశి Cfg_ఖండం& bb = static_cast<స్థిరరాశి Cfg_ఖండం&>(vtx);
        ఒకవేళ (!bb.isBranch()) కొనసాగు;
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vtx.బహిర్గతం_అంచులు()) {
            స్థిరరాశి Cfg_ఖండం& succ = static_cast<స్థిరరాశి Cfg_ఖండం&>(*అంచు.శిఖరం());
            ఒకవేళ (!succ.isJoin()) కొనసాగు;
            critical_అంచులు.emplace_వెనుక(static_cast<Cfg_అంచు*>(&అంచు));
        }
    }
    // Insert blocks
    కొరకు (Cfg_అంచు* స్థిరరాశి edgep : critical_అంచులు) {
        Cfg_ఖండం* స్థిరరాశి newp = కూడు_ఖండం();
        కూడు_తీసుకున్న_అంచు(newp, edgep->dstp());
        edgep->relink_శిఖరం(newp);
    }

    ఒకవేళ (m_nEdits != m_n_చివరి_Ordered) rpo_ఖండాలు();
    ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 9) dumpDotFilePrefixed("cfg-breakCriticalEdges");
}

// Given a branching basic block, if the sub-graph below this branch, up until
// the point where all of its control flow path convertes is series-parallel,
// then return the (potentially newly created) basic block with exactly 2
// predecessors where the two control flow paths from this branch have joined.
// If the relevant sub-graph is not series-parallel (there is a control flow
// path between the branches, or to a path not dominated by the given branch),
// then return nullptr. Cached results in the given map
Cfg_ఖండం* Cfg_లేఖాచిత్రం::getOrCreateTwoWayJoinFor(Cfg_ఖండం* bbp) {
    UASSERT_OBJ(bbp->isBranch(), bbp, "Not a branch");

    // Mark visited
    UASSERT_OBJ(!bbp->user(), bbp, "Should not visit twice");
    bbp->user(1);

    // We need the edge converting to a join block along both path. This is how we find it:
    స్థిరరాశి స్వయంచాలక chase_అంచు = [&](Cfg_అంచు* edgep) -> Cfg_అంచు* {
        అయితే (నిజం) {
            Cfg_ఖండం* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            ఒకవేళ (dstp->isJoin()) తిరిగి edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            అయితే (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                ఒకవేళ (!dstp) తిరిగి శూన్యసూచిక;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->తీసుకున్న_Edgep();
        }
    };

    // Walk down both paths
    Cfg_అంచు* స్థిరరాశి తీసుకున్న_Edgep = chase_అంచు(bbp->తీసుకున్న_Edgep());
    ఒకవేళ (!తీసుకున్న_Edgep) తిరిగి శూన్యసూచిక;
    Cfg_అంచు* స్థిరరాశి untknEdgep = chase_అంచు(bbp->untknEdgep());
    ఒకవేళ (!untknEdgep) తిరిగి శూన్యసూచిక;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    ఒకవేళ (తీసుకున్న_Edgep->dstp() != untknEdgep->dstp()) తిరిగి శూన్యసూచిక;

    // Pick up the common successor
    Cfg_ఖండం* స్థిరరాశి succp = తీసుకున్న_Edgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    ఒకవేళ (succp->isTwoWayJoin()) తిరిగి succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    Cfg_ఖండం* స్థిరరాశి joinp = కూడు_ఖండం();
    కూడు_తీసుకున్న_అంచు(joinp, succp);
    తీసుకున్న_Edgep->relink_శిఖరం(joinp);
    untknEdgep->relink_శిఖరం(joinp);
    తిరిగి joinp;
}

తార్కిక Cfg_లేఖాచిత్రం::చొప్పించు_Two_Way_Joins() {
    // Reset marks
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& v : vertices()) v.user(0);

    తార్కిక isSeriesParallel = నిజం;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& v : vertices()) {
        Cfg_ఖండం& bb = static_cast<Cfg_ఖండం&>(v);
        // Skip if already visited
        ఒకవేళ (bb.user()) కొనసాగు;
        // Skip if not a branch
        ఒకవేళ (!bb.isBranch()) కొనసాగు;
        // Fix it up, record if failed
        ఒకవేళ (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = అబద్ధం;
    }

    ఒకవేళ (m_nEdits != m_n_చివరి_Ordered) rpo_ఖండాలు();
    ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    తిరిగి isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

స్థిరరాశి Cfg_ఖండం* Cfg_Dominator_వృక్షం::intersect(స్థిరరాశి Cfg_ఖండం* ap, స్థిరరాశి Cfg_ఖండం* bp) {
    అయితే (ap != bp) {
        అయితే (*ap > *bp) ap = m_bb2Idom[*ap];
        అయితే (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    తిరిగి ap;
}

Cfg_Dominator_వృక్షం::Cfg_Dominator_వృక్షం(స్థిరరాశి Cfg_లేఖాచిత్రం& cfg)
    : m_bb2Idom{cfg.make_ఖండం_Map<స్థిరరాశి Cfg_ఖండం*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    కొరకు (తార్కిక మారింది = నిజం; మారింది;) {
        మారింది = అబద్ధం;
        // For each vertex except enter block
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : cfg.vertices()) {
            స్థిరరాశి Cfg_ఖండం& curr = static_cast<స్థిరరాశి Cfg_ఖండం&>(vtx);
            ఒకవేళ (curr.isEnter()) కొనసాగు;  // Skip entry block

            // For each predecessor of current block
            స్థిరరాశి Cfg_ఖండం* idom = శూన్యసూచిక;
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : curr.లోపల_అంచులు()) {
                స్థిరరాశి Cfg_ఖండం& pred = static_cast<స్థిరరాశి Cfg_ఖండం&>(*అంచు.fromp());
                // Skip if perdecessor not yet processed
                ఒకవేళ (!m_bb2Idom[pred]) కొనసాగు;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            ఒకవేళ (idom == m_bb2Idom[curr]) కొనసాగు;
            m_bb2Idom[curr] = idom;
            మారింది = నిజం;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = శూన్యసూచిక;
}
