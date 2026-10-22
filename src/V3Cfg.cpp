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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Cfg.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3EmitV.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// CfgBlock method definitions

std::string CfgBlock::name() ধ্রুবক {
    std::stringstream ss;
    ss << "BB " + std::to_string(id()) + ":\n";
    জন্য (Ast_গ্রন্থি* nodep : m_stmtps) {
        যদি (ধ্রুবক AstIf* ধ্রুবক ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debug_Verilog_For_বৃক্ষ(ifp->condp(), ss);
            ss << ") ...";
        } অন্যথা যদি (ধ্রুবক AstLoopTest* ধ্রুবক testp = VN_CAST(nodep, LoopTest)) {
            ss << "if  (!";
            V3EmitV::debug_Verilog_For_বৃক্ষ(testp->condp(), ss);
            ss << ") break;";
        } অন্যথা {
            V3EmitV::debug_Verilog_For_বৃক্ষ(nodep, ss);
        }
    }
    std::string text = VString::replaceSubstr(
        VString::replaceSubstr(ss.str(), "\n", "\\l        "), "\"", "\\\"");
    যদি (isEnter()) text = "**ENTER**\n" + text;
    যদি (isExit()) text = text + "\n**EXIT**";
    ফেরত text;
}
std::string CfgBlock::dotShape() ধ্রুবক { ফেরত "rect"; }
std::string CfgBlock::dotRank() ধ্রুবক {
    যদি (isEnter()) ফেরত "source";
    যদি (isExit()) ফেরত "sink";
    ফেরত "";
}

//######################################################################
// CfgEdge method definitions

std::string CfgEdge::dotLabel() ধ্রুবক {
    std::string label = "E" + std::to_string(id());
    ধ্রুবক CfgEdge* ধ্রুবক untknp = srcp()->untknEdgep();
    যদি (এই == untknp) {
        label += " / F";
    } অন্যথা যদি (untknp) {
        label += " / T";
    }
    ফেরত label;
}

//######################################################################
// CfgGraph method definitions

স্থির শূন্য cfgOrderVisitBlock(std::vector<CfgBlock*>& postOrderEnumeration, CfgBlock* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    যদি (CfgBlock* ধ্রুবক takenp = bbp->takenp()) {
        যদি (!takenp->user()) cfgOrderVisitBlock(postOrderEnumeration, takenp);
        যদি (CfgBlock* ধ্রুবক untknp = bbp->untknp()) {
            যদি (!untknp->user()) cfgOrderVisitBlock(postOrderEnumeration, untknp);
        }
    }
    // Add to post order enumeration
    postOrderEnumeration.emplace_back(bbp);
};

শূন্য CfgGraph::rpoBlocks() {
    UASSERT_OBJ(m_nEdits != m_nLastOrdered, m_enterp, "Redundant 'CfgGraph::order' call");
    m_nLastOrdered = m_nEdits;

    // Reset marks
    জন্য (V3GraphVertex& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::vector<CfgBlock*> postOrderEnumeration;
    postOrderEnumeration.reserve(m_nBlocks);
    cfgOrderVisitBlock(postOrderEnumeration, m_enterp);
    UASSERT_OBJ(postOrderEnumeration.আকার_2() == m_nBlocks, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    জন্য (আকার_t i = 0; i < postOrderEnumeration.আকার_2(); ++i) {
        CfgBlock* ধ্রুবক bbp = postOrderEnumeration[m_nBlocks - 1 - i];
        bbp->m_rpo_সংখ্যা = i;
        vertices().unlink(bbp);
        vertices().linkBack(bbp);
    }

    // Assign edge IDs
    আকার_t edge_গুনতি = 0;
    জন্য (V3GraphVertex& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        জন্য (V3GraphEdge& e : v.outEdges()) static_cast<CfgEdge&>(e).m_id = edge_গুনতি++;
    }
    UASSERT_OBJ(edge_গুনতি == m_nEdges, m_enterp, "Inconsistent edge count");
}

যৌক্তিক CfgGraph::containsLoop() ধ্রুবক {
    জন্য (ধ্রুবক V3GraphVertex& vtx : vertices()) {
        ধ্রুবক CfgBlock& current = static_cast<ধ্রুবক CfgBlock&>(vtx);
        জন্য (ধ্রুবক V3GraphEdge& edge : current.outEdges()) {
            ধ্রুবক CfgBlock& successor = *static_cast<ধ্রুবক CfgBlock*>(edge.শীর্ষ());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            যদি (successor.id() < current.id()) ফেরত সত্য;
        }
    }
    ফেরত মিথ্যা;
}

শূন্য CfgGraph::minimize() {
    // Remove empty blocks (except enter and exit)
    জন্য (V3GraphVertex* ধ্রুবক vtxp : vertices().unlinkable()) {
        CfgBlock* ধ্রুবক bbp = static_cast<CfgBlock*>(vtxp);
        যদি (bbp->isEnter()) চালু;
        যদি (bbp->isExit()) চালু;
        যদি (!bbp->stmtps().empty()) চালু;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        CfgBlock* ধ্রুবক succp = bbp->takenp();
        জন্য (V3GraphEdge* ধ্রুবক edgep : bbp->inEdges().unlinkable()) edgep->relink_শীর্ষ(succp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(bbp->unlinkDelete(এই), bbp);
    }

    // Combine sequential blocks
    জন্য (V3GraphVertex* ধ্রুবক vtxp : vertices().unlinkable()) {
        CfgBlock* ধ্রুবক srcp = static_cast<CfgBlock*>(vtxp);
        যদি (srcp->isExit()) চালু;
        যদি (srcp->isBranch()) চালু;
        CfgBlock* ধ্রুবক dstp = srcp->takenp();
        যদি (dstp->isJoin()) চালু;
        // Combine them
        যদি (srcp->isEnter()) m_enterp = dstp;
        std::vector<Ast_গ্রন্থি_Stmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.আকার_2() + dstp->m_stmtps.আকার_2());
        stmtps.ঢোকাও(stmtps.end(), dstp->m_stmtps.begin(), dstp->m_stmtps.end());
        dstp->m_stmtps = std::move(stmtps);
        জন্য (V3GraphEdge* ধ্রুবক edgep : srcp->inEdges().unlinkable()) edgep->relink_শীর্ষ(dstp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(srcp->unlinkDelete(এই), srcp);
    }

    যদি (m_nEdits != m_nLastOrdered) rpoBlocks();
    যদি (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

শূন্য CfgGraph::breakCriticalEdges() {
    // Gather critical edges
    std::vector<CfgEdge*> criticalEdges;
    criticalEdges.reserve(m_nEdges);
    জন্য (V3GraphVertex& vtx : vertices()) {
        ধ্রুবক CfgBlock& bb = static_cast<ধ্রুবক CfgBlock&>(vtx);
        যদি (!bb.isBranch()) চালু;
        জন্য (V3GraphEdge& edge : vtx.outEdges()) {
            ধ্রুবক CfgBlock& succ = static_cast<ধ্রুবক CfgBlock&>(*edge.শীর্ষ());
            যদি (!succ.isJoin()) চালু;
            criticalEdges.emplace_back(static_cast<CfgEdge*>(&edge));
        }
    }
    // Insert blocks
    জন্য (CfgEdge* ধ্রুবক edgep : criticalEdges) {
        CfgBlock* ধ্রুবক newp = addBlock();
        addTakenEdge(newp, edgep->dstp());
        edgep->relink_শীর্ষ(newp);
    }

    যদি (m_nEdits != m_nLastOrdered) rpoBlocks();
    যদি (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-breakCriticalEdges");
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
    ধ্রুবক auto chaseEdge = [&](CfgEdge* edgep) -> CfgEdge* {
        যখন (সত্য) {
            CfgBlock* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            যদি (dstp->isJoin()) ফেরত edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            যখন (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                যদি (!dstp) ফেরত nullptr;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->takenEdgep();
        }
    };

    // Walk down both paths
    CfgEdge* ধ্রুবক takenEdgep = chaseEdge(bbp->takenEdgep());
    যদি (!takenEdgep) ফেরত nullptr;
    CfgEdge* ধ্রুবক untknEdgep = chaseEdge(bbp->untknEdgep());
    যদি (!untknEdgep) ফেরত nullptr;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    যদি (takenEdgep->dstp() != untknEdgep->dstp()) ফেরত nullptr;

    // Pick up the common successor
    CfgBlock* ধ্রুবক succp = takenEdgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    যদি (succp->isTwoWayJoin()) ফেরত succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    CfgBlock* ধ্রুবক joinp = addBlock();
    addTakenEdge(joinp, succp);
    takenEdgep->relink_শীর্ষ(joinp);
    untknEdgep->relink_শীর্ষ(joinp);
    ফেরত joinp;
}

যৌক্তিক CfgGraph::ঢোকাও_Two_Way_Joins() {
    // Reset marks
    জন্য (V3GraphVertex& v : vertices()) v.user(0);

    যৌক্তিক isSeriesParallel = সত্য;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    জন্য (V3GraphVertex& v : vertices()) {
        CfgBlock& bb = static_cast<CfgBlock&>(v);
        // Skip if already visited
        যদি (bb.user()) চালু;
        // Skip if not a branch
        যদি (!bb.isBranch()) চালু;
        // Fix it up, record if failed
        যদি (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = মিথ্যা;
    }

    যদি (m_nEdits != m_nLastOrdered) rpoBlocks();
    যদি (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    ফেরত isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

ধ্রুবক CfgBlock* Cfg_Dominator_বৃক্ষ::intersect(ধ্রুবক CfgBlock* ap, ধ্রুবক CfgBlock* bp) {
    যখন (ap != bp) {
        যখন (*ap > *bp) ap = m_bb2Idom[*ap];
        যখন (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    ফেরত ap;
}

Cfg_Dominator_বৃক্ষ::Cfg_Dominator_বৃক্ষ(ধ্রুবক CfgGraph& cfg)
    : m_bb2Idom{cfg.makeBlockMap<ধ্রুবক CfgBlock*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    জন্য (যৌক্তিক changed = সত্য; changed;) {
        changed = মিথ্যা;
        // For each vertex except enter block
        জন্য (ধ্রুবক V3GraphVertex& vtx : cfg.vertices()) {
            ধ্রুবক CfgBlock& curr = static_cast<ধ্রুবক CfgBlock&>(vtx);
            যদি (curr.isEnter()) চালু;  // Skip entry block

            // For each predecessor of current block
            ধ্রুবক CfgBlock* idom = nullptr;
            জন্য (ধ্রুবক V3GraphEdge& edge : curr.inEdges()) {
                ধ্রুবক CfgBlock& pred = static_cast<ধ্রুবক CfgBlock&>(*edge.fromp());
                // Skip if perdecessor not yet processed
                যদি (!m_bb2Idom[pred]) চালু;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            যদি (idom == m_bb2Idom[curr]) চালু;
            m_bb2Idom[curr] = idom;
            changed = সত্য;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = nullptr;
}
