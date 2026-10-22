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

std::string CfgBlock::name() ध्रुव {
    std::stringstream ss;
    ss << "BB " + std::to_string(id()) + ":\n";
    प्रति (Ast_गाठ* nodep : m_stmtps) {
        यदि (ध्रुव AstIf* ध्रुव ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debug_Verilog_For_वृक्ष(ifp->condp(), ss);
            ss << ") ...";
        } अन्यथा यदि (ध्रुव AstLoopTest* ध्रुव testp = VN_CAST(nodep, LoopTest)) {
            ss << "if  (!";
            V3EmitV::debug_Verilog_For_वृक्ष(testp->condp(), ss);
            ss << ") break;";
        } अन्यथा {
            V3EmitV::debug_Verilog_For_वृक्ष(nodep, ss);
        }
    }
    std::string text = VString::replaceSubstr(
        VString::replaceSubstr(ss.str(), "\n", "\\l        "), "\"", "\\\"");
    यदि (isEnter()) text = "**ENTER**\n" + text;
    यदि (isExit()) text = text + "\n**EXIT**";
    प्रत्यागम text;
}
std::string CfgBlock::dotShape() ध्रुव { प्रत्यागम "rect"; }
std::string CfgBlock::dotRank() ध्रुव {
    यदि (isEnter()) प्रत्यागम "source";
    यदि (isExit()) प्रत्यागम "sink";
    प्रत्यागम "";
}

//######################################################################
// CfgEdge method definitions

std::string CfgEdge::dotLabel() ध्रुव {
    std::string label = "E" + std::to_string(id());
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

स्थिर शून्य cfgOrderVisitBlock(std::vector<CfgBlock*>& postOrderEnumeration, CfgBlock* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    यदि (CfgBlock* ध्रुव takenp = bbp->takenp()) {
        यदि (!takenp->user()) cfgOrderVisitBlock(postOrderEnumeration, takenp);
        यदि (CfgBlock* ध्रुव untknp = bbp->untknp()) {
            यदि (!untknp->user()) cfgOrderVisitBlock(postOrderEnumeration, untknp);
        }
    }
    // Add to post order enumeration
    postOrderEnumeration.emplace_back(bbp);
};

शून्य CfgGraph::rpoBlocks() {
    UASSERT_OBJ(m_nEdits != m_nLastOrdered, m_enterp, "Redundant 'CfgGraph::order' call");
    m_nLastOrdered = m_nEdits;

    // Reset marks
    प्रति (V3GraphVertex& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::vector<CfgBlock*> postOrderEnumeration;
    postOrderEnumeration.reserve(m_nBlocks);
    cfgOrderVisitBlock(postOrderEnumeration, m_enterp);
    UASSERT_OBJ(postOrderEnumeration.size() == m_nBlocks, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    प्रति (size_t i = 0; i < postOrderEnumeration.size(); ++i) {
        CfgBlock* ध्रुव bbp = postOrderEnumeration[m_nBlocks - 1 - i];
        bbp->m_rpo_संख्या = i;
        vertices().unlink(bbp);
        vertices().linkBack(bbp);
    }

    // Assign edge IDs
    size_t edge_मोजणी = 0;
    प्रति (V3GraphVertex& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        प्रति (V3GraphEdge& e : v.outEdges()) static_cast<CfgEdge&>(e).m_id = edge_मोजणी++;
    }
    UASSERT_OBJ(edge_मोजणी == m_nEdges, m_enterp, "Inconsistent edge count");
}

तार्किक CfgGraph::containsLoop() ध्रुव {
    प्रति (ध्रुव V3GraphVertex& vtx : vertices()) {
        ध्रुव CfgBlock& current = static_cast<ध्रुव CfgBlock&>(vtx);
        प्रति (ध्रुव V3GraphEdge& edge : current.outEdges()) {
            ध्रुव CfgBlock& successor = *static_cast<ध्रुव CfgBlock*>(edge.शीर्ष());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            यदि (successor.id() < current.id()) प्रत्यागम सत्य;
        }
    }
    प्रत्यागम असत्य;
}

शून्य CfgGraph::minimize() {
    // Remove empty blocks (except enter and exit)
    प्रति (V3GraphVertex* ध्रुव vtxp : vertices().unlinkable()) {
        CfgBlock* ध्रुव bbp = static_cast<CfgBlock*>(vtxp);
        यदि (bbp->isEnter()) अनुवर्त;
        यदि (bbp->isExit()) अनुवर्त;
        यदि (!bbp->stmtps().empty()) अनुवर्त;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        CfgBlock* ध्रुव succp = bbp->takenp();
        प्रति (V3GraphEdge* ध्रुव edgep : bbp->inEdges().unlinkable()) edgep->relink_शीर्ष(succp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(bbp->unlinkDelete(एतत्), bbp);
    }

    // Combine sequential blocks
    प्रति (V3GraphVertex* ध्रुव vtxp : vertices().unlinkable()) {
        CfgBlock* ध्रुव srcp = static_cast<CfgBlock*>(vtxp);
        यदि (srcp->isExit()) अनुवर्त;
        यदि (srcp->isBranch()) अनुवर्त;
        CfgBlock* ध्रुव dstp = srcp->takenp();
        यदि (dstp->isJoin()) अनुवर्त;
        // Combine them
        यदि (srcp->isEnter()) m_enterp = dstp;
        std::vector<Ast_गाठ_Stmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.size() + dstp->m_stmtps.size());
        stmtps.घाला(stmtps.end(), dstp->m_stmtps.begin(), dstp->m_stmtps.end());
        dstp->m_stmtps = std::move(stmtps);
        प्रति (V3GraphEdge* ध्रुव edgep : srcp->inEdges().unlinkable()) edgep->relink_शीर्ष(dstp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(srcp->unlinkDelete(एतत्), srcp);
    }

    यदि (m_nEdits != m_nLastOrdered) rpoBlocks();
    यदि (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

शून्य CfgGraph::breakCriticalEdges() {
    // Gather critical edges
    std::vector<CfgEdge*> criticalEdges;
    criticalEdges.reserve(m_nEdges);
    प्रति (V3GraphVertex& vtx : vertices()) {
        ध्रुव CfgBlock& bb = static_cast<ध्रुव CfgBlock&>(vtx);
        यदि (!bb.isBranch()) अनुवर्त;
        प्रति (V3GraphEdge& edge : vtx.outEdges()) {
            ध्रुव CfgBlock& succ = static_cast<ध्रुव CfgBlock&>(*edge.शीर्ष());
            यदि (!succ.isJoin()) अनुवर्त;
            criticalEdges.emplace_back(static_cast<CfgEdge*>(&edge));
        }
    }
    // Insert blocks
    प्रति (CfgEdge* ध्रुव edgep : criticalEdges) {
        CfgBlock* ध्रुव newp = addBlock();
        addTakenEdge(newp, edgep->dstp());
        edgep->relink_शीर्ष(newp);
    }

    यदि (m_nEdits != m_nLastOrdered) rpoBlocks();
    यदि (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-breakCriticalEdges");
}

// Given a branching basic block, if the sub-graph below this branch, up until
// the point where all of its control flow path convertes is series-parallel,
// then return the (potentially newly created) basic block with exactly 2
// predecessors where the two control flow paths from this branch have joined.
// If the relevant sub-graph is not series-parallel (there is a control flow
// path between the branches, or to a path not dominated by the given branch),
// then return nullptr. Cached results in the given map
CfgBlock* CfgGraph::getOrCreateTwoWayJoinFor(CfgBlock* bbp) {
    UASSERT_OBJ(bbp->isBranch(), bbp, "Not a branch");

    // Mark visited
    UASSERT_OBJ(!bbp->user(), bbp, "Should not visit twice");
    bbp->user(1);

    // We need the edge converting to a join block along both path. This is how we find it:
    ध्रुव auto chaseEdge = [&](CfgEdge* edgep) -> CfgEdge* {
        यावत् (सत्य) {
            CfgBlock* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            यदि (dstp->isJoin()) प्रत्यागम edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            यावत् (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                यदि (!dstp) प्रत्यागम nullptr;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->takenEdgep();
        }
    };

    // Walk down both paths
    CfgEdge* ध्रुव takenEdgep = chaseEdge(bbp->takenEdgep());
    यदि (!takenEdgep) प्रत्यागम nullptr;
    CfgEdge* ध्रुव untknEdgep = chaseEdge(bbp->untknEdgep());
    यदि (!untknEdgep) प्रत्यागम nullptr;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    यदि (takenEdgep->dstp() != untknEdgep->dstp()) प्रत्यागम nullptr;

    // Pick up the common successor
    CfgBlock* ध्रुव succp = takenEdgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    यदि (succp->isTwoWayJoin()) प्रत्यागम succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    CfgBlock* ध्रुव joinp = addBlock();
    addTakenEdge(joinp, succp);
    takenEdgep->relink_शीर्ष(joinp);
    untknEdgep->relink_शीर्ष(joinp);
    प्रत्यागम joinp;
}

तार्किक CfgGraph::घाला_Two_Way_Joins() {
    // Reset marks
    प्रति (V3GraphVertex& v : vertices()) v.user(0);

    तार्किक isSeriesParallel = सत्य;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    प्रति (V3GraphVertex& v : vertices()) {
        CfgBlock& bb = static_cast<CfgBlock&>(v);
        // Skip if already visited
        यदि (bb.user()) अनुवर्त;
        // Skip if not a branch
        यदि (!bb.isBranch()) अनुवर्त;
        // Fix it up, record if failed
        यदि (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = असत्य;
    }

    यदि (m_nEdits != m_nLastOrdered) rpoBlocks();
    यदि (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    प्रत्यागम isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

ध्रुव CfgBlock* Cfg_Dominator_वृक्ष::intersect(ध्रुव CfgBlock* ap, ध्रुव CfgBlock* bp) {
    यावत् (ap != bp) {
        यावत् (*ap > *bp) ap = m_bb2Idom[*ap];
        यावत् (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    प्रत्यागम ap;
}

Cfg_Dominator_वृक्ष::Cfg_Dominator_वृक्ष(ध्रुव CfgGraph& cfg)
    : m_bb2Idom{cfg.makeBlockMap<ध्रुव CfgBlock*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    प्रति (तार्किक changed = सत्य; changed;) {
        changed = असत्य;
        // For each vertex except enter block
        प्रति (ध्रुव V3GraphVertex& vtx : cfg.vertices()) {
            ध्रुव CfgBlock& curr = static_cast<ध्रुव CfgBlock&>(vtx);
            यदि (curr.isEnter()) अनुवर्त;  // Skip entry block

            // For each predecessor of current block
            ध्रुव CfgBlock* idom = nullptr;
            प्रति (ध्रुव V3GraphEdge& edge : curr.inEdges()) {
                ध्रुव CfgBlock& pred = static_cast<ध्रुव CfgBlock&>(*edge.fromp());
                // Skip if perdecessor not yet processed
                यदि (!m_bb2Idom[pred]) अनुवर्त;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            यदि (idom == m_bb2Idom[curr]) अनुवर्त;
            m_bb2Idom[curr] = idom;
            changed = सत्य;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = nullptr;
}
