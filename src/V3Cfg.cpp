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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Cfg.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3EmitV.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// CfgBlock method definitions

std::string Cfg_खण्डम्::name() ध्रुव {
    std::stringstream ss;
    ss << "BB " + std::गन्तव्यम्_string(id()) + ":\n";
    प्रति (Ast_ग्रन्थिः* nodep : m_stmtps) {
        यदि (ध्रुव AstIf* ध्रुव ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debug_Verilog_For_वृक्षः(ifp->condp(), ss);
            ss << ") ...";
        } अन्यथा यदि (ध्रुव AstLoopTest* ध्रुव testp = VN_CAST(nodep, LoopTest)) {
            ss << "if  (!";
            V3EmitV::debug_Verilog_For_वृक्षः(testp->condp(), ss);
            ss << ") break;";
        } अन्यथा {
            V3EmitV::debug_Verilog_For_वृक्षः(nodep, ss);
        }
    }
    std::string text = VString::replaceSubstr(
        VString::replaceSubstr(ss.str(), "\n", "\\l        "), "\"", "\\\"");
    यदि (isEnter()) text = "**ENTER**\n" + text;
    यदि (isExit()) text = text + "\n**EXIT**";
    प्रत्यागम text;
}
std::string Cfg_खण्डम्::dotShape() ध्रुव { प्रत्यागम "rect"; }
std::string Cfg_खण्डम्::dotRank() ध्रुव {
    यदि (isEnter()) प्रत्यागम "source";
    यदि (isExit()) प्रत्यागम "sink";
    प्रत्यागम "";
}

//######################################################################
// CfgEdge method definitions

std::string CfgEdge::dotLabel() ध्रुव {
    std::string label = "E" + std::गन्तव्यम्_string(id());
    ध्रुव CfgEdge* ध्रुव untknp = srcp()->untknEdgep();
    यदि (एतत् == untknp) {
        label += " / F";
    } अन्यथा यदि (untknp) {
        label += " / T";
    }
    प्रत्यागम label;
}

//######################################################################
// CfgGraph method definitions

स्थिर शून्य cfg_क्रमः_दर्शय_खण्डम्(std::सदिशम्<Cfg_खण्डम्*>& post_क्रमः_Enumeration, Cfg_खण्डम्* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    यदि (Cfg_खण्डम्* ध्रुव takenp = bbp->takenp()) {
        यदि (!takenp->user()) cfg_क्रमः_दर्शय_खण्डम्(post_क्रमः_Enumeration, takenp);
        यदि (Cfg_खण्डम्* ध्रुव untknp = bbp->untknp()) {
            यदि (!untknp->user()) cfg_क्रमः_दर्शय_खण्डम्(post_क्रमः_Enumeration, untknp);
        }
    }
    // Add to post order enumeration
    post_क्रमः_Enumeration.emplace_पृष्ठभागः(bbp);
};

शून्य CfgGraph::rpo_खण्डानि() {
    UASSERT_OBJ(m_nEdits != m_n_चरमः_Ordered, m_enterp, "Redundant 'CfgGraph::order' call");
    m_n_चरमः_Ordered = m_nEdits;

    // Reset marks
    प्रति (V3GraphVertex& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::सदिशम्<Cfg_खण्डम्*> post_क्रमः_Enumeration;
    post_क्रमः_Enumeration.reserve(m_n_खण्डानि);
    cfg_क्रमः_दर्शय_खण्डम्(post_क्रमः_Enumeration, m_enterp);
    UASSERT_OBJ(post_क्रमः_Enumeration.आकार() == m_n_खण्डानि, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    प्रति (आकार_t i = 0; i < post_क्रमः_Enumeration.आकार(); ++i) {
        Cfg_खण्डम्* ध्रुव bbp = post_क्रमः_Enumeration[m_n_खण्डानि - 1 - i];
        bbp->m_rpo_सङ्ख्या = i;
        vertices().unlink(bbp);
        vertices().link_पृष्ठभागः(bbp);
    }

    // Assign edge IDs
    आकार_t edge_गणनम् = 0;
    प्रति (V3GraphVertex& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        प्रति (V3GraphEdge& e : v.बहिर्गमनम्_धाराः()) static_cast<CfgEdge&>(e).m_id = edge_गणनम्++;
    }
    UASSERT_OBJ(edge_गणनम् == m_n_धाराः, m_enterp, "Inconsistent edge count");
}

तार्किक CfgGraph::अन्तर्भवति_Loop() ध्रुव {
    प्रति (ध्रुव V3GraphVertex& vtx : vertices()) {
        ध्रुव Cfg_खण्डम्& current = static_cast<ध्रुव Cfg_खण्डम्&>(vtx);
        प्रति (ध्रुव V3GraphEdge& edge : current.बहिर्गमनम्_धाराः()) {
            ध्रुव Cfg_खण्डम्& successor = *static_cast<ध्रुव Cfg_खण्डम्*>(edge.शिखरम्());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            यदि (successor.id() < current.id()) प्रत्यागम सत्य;
        }
    }
    प्रत्यागम असत्य;
}

शून्य CfgGraph::minimize() {
    // Remove empty blocks (except enter and exit)
    प्रति (V3GraphVertex* ध्रुव vtxp : vertices().unlinkable()) {
        Cfg_खण्डम्* ध्रुव bbp = static_cast<Cfg_खण्डम्*>(vtxp);
        यदि (bbp->isEnter()) अनुवर्त;
        यदि (bbp->isExit()) अनुवर्त;
        यदि (!bbp->stmtps().रिक्तानि()) अनुवर्त;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        Cfg_खण्डम्* ध्रुव succp = bbp->takenp();
        प्रति (V3GraphEdge* ध्रुव edgep : bbp->अन्तर्_धाराः().unlinkable()) edgep->relink_शिखरम्(succp);
        ++m_nEdits;
        --m_n_धाराः;
        --m_n_खण्डानि;
        VL_DO_DANGLING(bbp->unlinkDelete(एतत्), bbp);
    }

    // Combine sequential blocks
    प्रति (V3GraphVertex* ध्रुव vtxp : vertices().unlinkable()) {
        Cfg_खण्डम्* ध्रुव srcp = static_cast<Cfg_खण्डम्*>(vtxp);
        यदि (srcp->isExit()) अनुवर्त;
        यदि (srcp->isBranch()) अनुवर्त;
        Cfg_खण्डम्* ध्रुव dstp = srcp->takenp();
        यदि (dstp->isJoin()) अनुवर्त;
        // Combine them
        यदि (srcp->isEnter()) m_enterp = dstp;
        std::सदिशम्<Ast_ग्रन्थिः_Stmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.आकार() + dstp->m_stmtps.आकार());
        stmtps.निवेशय(stmtps.अन्तः_2(), dstp->m_stmtps.begin(), dstp->m_stmtps.अन्तः_2());
        dstp->m_stmtps = std::move(stmtps);
        प्रति (V3GraphEdge* ध्रुव edgep : srcp->अन्तर्_धाराः().unlinkable()) edgep->relink_शिखरम्(dstp);
        ++m_nEdits;
        --m_n_धाराः;
        --m_n_खण्डानि;
        VL_DO_DANGLING(srcp->unlinkDelete(एतत्), srcp);
    }

    यदि (m_nEdits != m_n_चरमः_Ordered) rpo_खण्डानि();
    यदि (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

शून्य CfgGraph::break_Critical_धाराः() {
    // Gather critical edges
    std::सदिशम्<CfgEdge*> critical_धाराः;
    critical_धाराः.reserve(m_n_धाराः);
    प्रति (V3GraphVertex& vtx : vertices()) {
        ध्रुव Cfg_खण्डम्& bb = static_cast<ध्रुव Cfg_खण्डम्&>(vtx);
        यदि (!bb.isBranch()) अनुवर्त;
        प्रति (V3GraphEdge& edge : vtx.बहिर्गमनम्_धाराः()) {
            ध्रुव Cfg_खण्डम्& succ = static_cast<ध्रुव Cfg_खण्डम्&>(*edge.शिखरम्());
            यदि (!succ.isJoin()) अनुवर्त;
            critical_धाराः.emplace_पृष्ठभागः(static_cast<CfgEdge*>(&edge));
        }
    }
    // Insert blocks
    प्रति (CfgEdge* ध्रुव edgep : critical_धाराः) {
        Cfg_खण्डम्* ध्रुव newp = सङ्कलय_खण्डम्();
        सङ्कलय_गृहीतम्_Edge(newp, edgep->dstp());
        edgep->relink_शिखरम्(newp);
    }

    यदि (m_nEdits != m_n_चरमः_Ordered) rpo_खण्डानि();
    यदि (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-breakCriticalEdges");
}

// Given a branching basic block, if the sub-graph below this branch, up until
// the point where all of its control flow path convertes is series-parallel,
// then return the (potentially newly created) basic block with exactly 2
// predecessors where the two control flow paths from this branch have joined.
// If the relevant sub-graph is not series-parallel (there is a control flow
// path between the branches, or to a path not dominated by the given branch),
// then return nullptr. Cached results in the given map
Cfg_खण्डम्* CfgGraph::getOrCreateTwoWayJoinFor(Cfg_खण्डम्* bbp) {
    UASSERT_OBJ(bbp->isBranch(), bbp, "Not a branch");

    // Mark visited
    UASSERT_OBJ(!bbp->user(), bbp, "Should not visit twice");
    bbp->user(1);

    // We need the edge converting to a join block along both path. This is how we find it:
    ध्रुव स्वचालित chaseEdge = [&](CfgEdge* edgep) -> CfgEdge* {
        यावत् (सत्य) {
            Cfg_खण्डम्* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            यदि (dstp->isJoin()) प्रत्यागम edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            यावत् (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                यदि (!dstp) प्रत्यागम शून्यसूचकः;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->गृहीतम्_Edgep();
        }
    };

    // Walk down both paths
    CfgEdge* ध्रुव गृहीतम्_Edgep = chaseEdge(bbp->गृहीतम्_Edgep());
    यदि (!गृहीतम्_Edgep) प्रत्यागम शून्यसूचकः;
    CfgEdge* ध्रुव untknEdgep = chaseEdge(bbp->untknEdgep());
    यदि (!untknEdgep) प्रत्यागम शून्यसूचकः;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    यदि (गृहीतम्_Edgep->dstp() != untknEdgep->dstp()) प्रत्यागम शून्यसूचकः;

    // Pick up the common successor
    Cfg_खण्डम्* ध्रुव succp = गृहीतम्_Edgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    यदि (succp->isTwoWayJoin()) प्रत्यागम succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    Cfg_खण्डम्* ध्रुव joinp = सङ्कलय_खण्डम्();
    सङ्कलय_गृहीतम्_Edge(joinp, succp);
    गृहीतम्_Edgep->relink_शिखरम्(joinp);
    untknEdgep->relink_शिखरम्(joinp);
    प्रत्यागम joinp;
}

तार्किक CfgGraph::निवेशय_Two_Way_Joins() {
    // Reset marks
    प्रति (V3GraphVertex& v : vertices()) v.user(0);

    तार्किक isSeriesParallel = सत्य;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    प्रति (V3GraphVertex& v : vertices()) {
        Cfg_खण्डम्& bb = static_cast<Cfg_खण्डम्&>(v);
        // Skip if already visited
        यदि (bb.user()) अनुवर्त;
        // Skip if not a branch
        यदि (!bb.isBranch()) अनुवर्त;
        // Fix it up, record if failed
        यदि (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = असत्य;
    }

    यदि (m_nEdits != m_n_चरमः_Ordered) rpo_खण्डानि();
    यदि (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    प्रत्यागम isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

ध्रुव Cfg_खण्डम्* Cfg_Dominator_वृक्षः::intersect(ध्रुव Cfg_खण्डम्* ap, ध्रुव Cfg_खण्डम्* bp) {
    यावत् (ap != bp) {
        यावत् (*ap > *bp) ap = m_bb2Idom[*ap];
        यावत् (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    प्रत्यागम ap;
}

Cfg_Dominator_वृक्षः::Cfg_Dominator_वृक्षः(ध्रुव CfgGraph& cfg)
    : m_bb2Idom{cfg.make_खण्डम्_Map<ध्रुव Cfg_खण्डम्*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    प्रति (तार्किक परिवर्तितम् = सत्य; परिवर्तितम्;) {
        परिवर्तितम् = असत्य;
        // For each vertex except enter block
        प्रति (ध्रुव V3GraphVertex& vtx : cfg.vertices()) {
            ध्रुव Cfg_खण्डम्& curr = static_cast<ध्रुव Cfg_खण्डम्&>(vtx);
            यदि (curr.isEnter()) अनुवर्त;  // Skip entry block

            // For each predecessor of current block
            ध्रुव Cfg_खण्डम्* idom = शून्यसूचकः;
            प्रति (ध्रुव V3GraphEdge& edge : curr.अन्तर्_धाराः()) {
                ध्रुव Cfg_खण्डम्& pred = static_cast<ध्रुव Cfg_खण्डम्&>(*edge.fromp());
                // Skip if perdecessor not yet processed
                यदि (!m_bb2Idom[pred]) अनुवर्त;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            यदि (idom == m_bb2Idom[curr]) अनुवर्त;
            m_bb2Idom[curr] = idom;
            परिवर्तितम् = सत्य;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = शून्यसूचकः;
}
