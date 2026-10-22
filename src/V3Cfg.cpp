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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Cfg.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3EmitV.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// CfgBlock method definitions

std::string CfgBlock::name() ಧ್ರುವ {
    std::stringstream ss;
    ss << "BB " + std::to_string(id()) + ":\n";
    ಪ್ರತಿ (Ast_ಗಂಟು* nodep : m_stmtps) {
        ಯದಿ (ಧ್ರುವ AstIf* ಧ್ರುವ ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debug_Verilog_For_ವೃಕ್ಷ(ifp->condp(), ss);
            ss << ") ...";
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstLoopTest* ಧ್ರುವ testp = VN_CAST(nodep, LoopTest)) {
            ss << "if  (!";
            V3EmitV::debug_Verilog_For_ವೃಕ್ಷ(testp->condp(), ss);
            ss << ") break;";
        } ಅನ್ಯಥಾ {
            V3EmitV::debug_Verilog_For_ವೃಕ್ಷ(nodep, ss);
        }
    }
    std::string text = VString::replaceSubstr(
        VString::replaceSubstr(ss.str(), "\n", "\\l        "), "\"", "\\\"");
    ಯದಿ (isEnter()) text = "**ENTER**\n" + text;
    ಯದಿ (isExit()) text = text + "\n**EXIT**";
    ಪ್ರತ್ಯಾಗಮ text;
}
std::string CfgBlock::dotShape() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ "rect"; }
std::string CfgBlock::dotRank() ಧ್ರುವ {
    ಯದಿ (isEnter()) ಪ್ರತ್ಯಾಗಮ "source";
    ಯದಿ (isExit()) ಪ್ರತ್ಯಾಗಮ "sink";
    ಪ್ರತ್ಯಾಗಮ "";
}

//######################################################################
// CfgEdge method definitions

std::string CfgEdge::dotLabel() ಧ್ರುವ {
    std::string label = "E" + std::to_string(id());
    ಧ್ರುವ CfgEdge* ಧ್ರುವ untknp = srcp()->untknEdgep();
    ಯದಿ (ಏತತ್ == untknp) {
        label += " / F";
    } ಅನ್ಯಥಾ ಯದಿ (untknp) {
        label += " / T";
    }
    ಪ್ರತ್ಯಾಗಮ label;
}

//######################################################################
// CfgGraph method definitions

ಸ್ಥಿರ ಶೂನ್ಯ cfgOrderVisitBlock(std::vector<CfgBlock*>& postOrderEnumeration, CfgBlock* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    ಯದಿ (CfgBlock* ಧ್ರುವ takenp = bbp->takenp()) {
        ಯದಿ (!takenp->user()) cfgOrderVisitBlock(postOrderEnumeration, takenp);
        ಯದಿ (CfgBlock* ಧ್ರುವ untknp = bbp->untknp()) {
            ಯದಿ (!untknp->user()) cfgOrderVisitBlock(postOrderEnumeration, untknp);
        }
    }
    // Add to post order enumeration
    postOrderEnumeration.emplace_back(bbp);
};

ಶೂನ್ಯ CfgGraph::rpoBlocks() {
    UASSERT_OBJ(m_nEdits != m_nLastOrdered, m_enterp, "Redundant 'CfgGraph::order' call");
    m_nLastOrdered = m_nEdits;

    // Reset marks
    ಪ್ರತಿ (V3GraphVertex& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::vector<CfgBlock*> postOrderEnumeration;
    postOrderEnumeration.reserve(m_nBlocks);
    cfgOrderVisitBlock(postOrderEnumeration, m_enterp);
    UASSERT_OBJ(postOrderEnumeration.size() == m_nBlocks, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    ಪ್ರತಿ (size_t i = 0; i < postOrderEnumeration.size(); ++i) {
        CfgBlock* ಧ್ರುವ bbp = postOrderEnumeration[m_nBlocks - 1 - i];
        bbp->m_rpo_ಸಂಖ್ಯೆ = i;
        vertices().unlink(bbp);
        vertices().linkBack(bbp);
    }

    // Assign edge IDs
    size_t edge_ಎಣಿಕೆ = 0;
    ಪ್ರತಿ (V3GraphVertex& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        ಪ್ರತಿ (V3GraphEdge& e : v.outEdges()) static_cast<CfgEdge&>(e).m_id = edge_ಎಣಿಕೆ++;
    }
    UASSERT_OBJ(edge_ಎಣಿಕೆ == m_nEdges, m_enterp, "Inconsistent edge count");
}

ತಾರ್ಕಿಕ CfgGraph::containsLoop() ಧ್ರುವ {
    ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : vertices()) {
        ಧ್ರುವ CfgBlock& current = static_cast<ಧ್ರುವ CfgBlock&>(vtx);
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : current.outEdges()) {
            ಧ್ರುವ CfgBlock& successor = *static_cast<ಧ್ರುವ CfgBlock*>(edge.ತುದಿ());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            ಯದಿ (successor.id() < current.id()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
    }
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
}

ಶೂನ್ಯ CfgGraph::minimize() {
    // Remove empty blocks (except enter and exit)
    ಪ್ರತಿ (V3GraphVertex* ಧ್ರುವ vtxp : vertices().unlinkable()) {
        CfgBlock* ಧ್ರುವ bbp = static_cast<CfgBlock*>(vtxp);
        ಯದಿ (bbp->isEnter()) ಅನುವರ್ತ;
        ಯದಿ (bbp->isExit()) ಅನುವರ್ತ;
        ಯದಿ (!bbp->stmtps().empty()) ಅನುವರ್ತ;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        CfgBlock* ಧ್ರುವ succp = bbp->takenp();
        ಪ್ರತಿ (V3GraphEdge* ಧ್ರುವ edgep : bbp->inEdges().unlinkable()) edgep->relink_ತುದಿ(succp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(bbp->unlinkDelete(ಏತತ್), bbp);
    }

    // Combine sequential blocks
    ಪ್ರತಿ (V3GraphVertex* ಧ್ರುವ vtxp : vertices().unlinkable()) {
        CfgBlock* ಧ್ರುವ srcp = static_cast<CfgBlock*>(vtxp);
        ಯದಿ (srcp->isExit()) ಅನುವರ್ತ;
        ಯದಿ (srcp->isBranch()) ಅನುವರ್ತ;
        CfgBlock* ಧ್ರುವ dstp = srcp->takenp();
        ಯದಿ (dstp->isJoin()) ಅನುವರ್ತ;
        // Combine them
        ಯದಿ (srcp->isEnter()) m_enterp = dstp;
        std::vector<Ast_ಗಂಟು_Stmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.size() + dstp->m_stmtps.size());
        stmtps.ಸೇರಿಸು(stmtps.end(), dstp->m_stmtps.begin(), dstp->m_stmtps.end());
        dstp->m_stmtps = std::move(stmtps);
        ಪ್ರತಿ (V3GraphEdge* ಧ್ರುವ edgep : srcp->inEdges().unlinkable()) edgep->relink_ತುದಿ(dstp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(srcp->unlinkDelete(ಏತತ್), srcp);
    }

    ಯದಿ (m_nEdits != m_nLastOrdered) rpoBlocks();
    ಯದಿ (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

ಶೂನ್ಯ CfgGraph::breakCriticalEdges() {
    // Gather critical edges
    std::vector<CfgEdge*> criticalEdges;
    criticalEdges.reserve(m_nEdges);
    ಪ್ರತಿ (V3GraphVertex& vtx : vertices()) {
        ಧ್ರುವ CfgBlock& bb = static_cast<ಧ್ರುವ CfgBlock&>(vtx);
        ಯದಿ (!bb.isBranch()) ಅನುವರ್ತ;
        ಪ್ರತಿ (V3GraphEdge& edge : vtx.outEdges()) {
            ಧ್ರುವ CfgBlock& succ = static_cast<ಧ್ರುವ CfgBlock&>(*edge.ತುದಿ());
            ಯದಿ (!succ.isJoin()) ಅನುವರ್ತ;
            criticalEdges.emplace_back(static_cast<CfgEdge*>(&edge));
        }
    }
    // Insert blocks
    ಪ್ರತಿ (CfgEdge* ಧ್ರುವ edgep : criticalEdges) {
        CfgBlock* ಧ್ರುವ newp = addBlock();
        addTakenEdge(newp, edgep->dstp());
        edgep->relink_ತುದಿ(newp);
    }

    ಯದಿ (m_nEdits != m_nLastOrdered) rpoBlocks();
    ಯದಿ (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-breakCriticalEdges");
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
    ಧ್ರುವ auto chaseEdge = [&](CfgEdge* edgep) -> CfgEdge* {
        ಯಾವತ್ (ಸತ್ಯ) {
            CfgBlock* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            ಯದಿ (dstp->isJoin()) ಪ್ರತ್ಯಾಗಮ edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            ಯಾವತ್ (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                ಯದಿ (!dstp) ಪ್ರತ್ಯಾಗಮ nullptr;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->takenEdgep();
        }
    };

    // Walk down both paths
    CfgEdge* ಧ್ರುವ takenEdgep = chaseEdge(bbp->takenEdgep());
    ಯದಿ (!takenEdgep) ಪ್ರತ್ಯಾಗಮ nullptr;
    CfgEdge* ಧ್ರುವ untknEdgep = chaseEdge(bbp->untknEdgep());
    ಯದಿ (!untknEdgep) ಪ್ರತ್ಯಾಗಮ nullptr;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    ಯದಿ (takenEdgep->dstp() != untknEdgep->dstp()) ಪ್ರತ್ಯಾಗಮ nullptr;

    // Pick up the common successor
    CfgBlock* ಧ್ರುವ succp = takenEdgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    ಯದಿ (succp->isTwoWayJoin()) ಪ್ರತ್ಯಾಗಮ succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    CfgBlock* ಧ್ರುವ joinp = addBlock();
    addTakenEdge(joinp, succp);
    takenEdgep->relink_ತುದಿ(joinp);
    untknEdgep->relink_ತುದಿ(joinp);
    ಪ್ರತ್ಯಾಗಮ joinp;
}

ತಾರ್ಕಿಕ CfgGraph::ಸೇರಿಸು_Two_Way_Joins() {
    // Reset marks
    ಪ್ರತಿ (V3GraphVertex& v : vertices()) v.user(0);

    ತಾರ್ಕಿಕ isSeriesParallel = ಸತ್ಯ;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    ಪ್ರತಿ (V3GraphVertex& v : vertices()) {
        CfgBlock& bb = static_cast<CfgBlock&>(v);
        // Skip if already visited
        ಯದಿ (bb.user()) ಅನುವರ್ತ;
        // Skip if not a branch
        ಯದಿ (!bb.isBranch()) ಅನುವರ್ತ;
        // Fix it up, record if failed
        ಯದಿ (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = ಅಸತ್ಯ;
    }

    ಯದಿ (m_nEdits != m_nLastOrdered) rpoBlocks();
    ಯದಿ (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    ಪ್ರತ್ಯಾಗಮ isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

ಧ್ರುವ CfgBlock* Cfg_Dominator_ವೃಕ್ಷ::intersect(ಧ್ರುವ CfgBlock* ap, ಧ್ರುವ CfgBlock* bp) {
    ಯಾವತ್ (ap != bp) {
        ಯಾವತ್ (*ap > *bp) ap = m_bb2Idom[*ap];
        ಯಾವತ್ (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    ಪ್ರತ್ಯಾಗಮ ap;
}

Cfg_Dominator_ವೃಕ್ಷ::Cfg_Dominator_ವೃಕ್ಷ(ಧ್ರುವ CfgGraph& cfg)
    : m_bb2Idom{cfg.makeBlockMap<ಧ್ರುವ CfgBlock*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    ಪ್ರತಿ (ತಾರ್ಕಿಕ changed = ಸತ್ಯ; changed;) {
        changed = ಅಸತ್ಯ;
        // For each vertex except enter block
        ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : cfg.vertices()) {
            ಧ್ರುವ CfgBlock& curr = static_cast<ಧ್ರುವ CfgBlock&>(vtx);
            ಯದಿ (curr.isEnter()) ಅನುವರ್ತ;  // Skip entry block

            // For each predecessor of current block
            ಧ್ರುವ CfgBlock* idom = nullptr;
            ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : curr.inEdges()) {
                ಧ್ರುವ CfgBlock& pred = static_cast<ಧ್ರುವ CfgBlock&>(*edge.fromp());
                // Skip if perdecessor not yet processed
                ಯದಿ (!m_bb2Idom[pred]) ಅನುವರ್ತ;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            ಯದಿ (idom == m_bb2Idom[curr]) ಅನುವರ್ತ;
            m_bb2Idom[curr] = idom;
            changed = ಸತ್ಯ;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = nullptr;
}
