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

std::string CfgBlock::name() ধ্রুব {
    std::stringstream ss;
    ss << "BB " + std::to_string(id()) + ":\n";
    প্রতি (Ast_গ্রন্থি* nodep : m_stmtps) {
        যদি (ধ্রুব AstIf* ধ্রুব ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debug_Verilog_For_বৃক্ষ(ifp->condp(), ss);
            ss << ") ...";
        } অন্যথা যদি (ধ্রুব AstLoopTest* ধ্রুব testp = VN_CAST(nodep, LoopTest)) {
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
    প্রত্যাগম text;
}
std::string CfgBlock::dotShape() ধ্রুব { প্রত্যাগম "rect"; }
std::string CfgBlock::dotRank() ধ্রুব {
    যদি (isEnter()) প্রত্যাগম "source";
    যদি (isExit()) প্রত্যাগম "sink";
    প্রত্যাগম "";
}

//######################################################################
// CfgEdge method definitions

std::string CfgEdge::dotLabel() ধ্রুব {
    std::string label = "E" + std::to_string(id());
    ধ্রুব CfgEdge* ধ্রুব untknp = srcp()->untknEdgep();
    যদি (এতত্ == untknp) {
        label += " / F";
    } অন্যথা যদি (untknp) {
        label += " / T";
    }
    প্রত্যাগম label;
}

//######################################################################
// CfgGraph method definitions

স্থির শূন্য cfgOrderVisitBlock(std::vector<CfgBlock*>& postOrderEnumeration, CfgBlock* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    যদি (CfgBlock* ধ্রুব takenp = bbp->takenp()) {
        যদি (!takenp->user()) cfgOrderVisitBlock(postOrderEnumeration, takenp);
        যদি (CfgBlock* ধ্রুব untknp = bbp->untknp()) {
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
    প্রতি (V3GraphVertex& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::vector<CfgBlock*> postOrderEnumeration;
    postOrderEnumeration.reserve(m_nBlocks);
    cfgOrderVisitBlock(postOrderEnumeration, m_enterp);
    UASSERT_OBJ(postOrderEnumeration.size() == m_nBlocks, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    প্রতি (size_t i = 0; i < postOrderEnumeration.size(); ++i) {
        CfgBlock* ধ্রুব bbp = postOrderEnumeration[m_nBlocks - 1 - i];
        bbp->m_rpo_সংখ্যা = i;
        vertices().unlink(bbp);
        vertices().linkBack(bbp);
    }

    // Assign edge IDs
    size_t edge_গুনতি = 0;
    প্রতি (V3GraphVertex& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        প্রতি (V3GraphEdge& e : v.outEdges()) static_cast<CfgEdge&>(e).m_id = edge_গুনতি++;
    }
    UASSERT_OBJ(edge_গুনতি == m_nEdges, m_enterp, "Inconsistent edge count");
}

তার্কিক CfgGraph::containsLoop() ধ্রুব {
    প্রতি (ধ্রুব V3GraphVertex& vtx : vertices()) {
        ধ্রুব CfgBlock& current = static_cast<ধ্রুব CfgBlock&>(vtx);
        প্রতি (ধ্রুব V3GraphEdge& edge : current.outEdges()) {
            ধ্রুব CfgBlock& successor = *static_cast<ধ্রুব CfgBlock*>(edge.শীর্ষ());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            যদি (successor.id() < current.id()) প্রত্যাগম সত্য;
        }
    }
    প্রত্যাগম অসত্য;
}

শূন্য CfgGraph::minimize() {
    // Remove empty blocks (except enter and exit)
    প্রতি (V3GraphVertex* ধ্রুব vtxp : vertices().unlinkable()) {
        CfgBlock* ধ্রুব bbp = static_cast<CfgBlock*>(vtxp);
        যদি (bbp->isEnter()) অনুবর্ত;
        যদি (bbp->isExit()) অনুবর্ত;
        যদি (!bbp->stmtps().empty()) অনুবর্ত;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        CfgBlock* ধ্রুব succp = bbp->takenp();
        প্রতি (V3GraphEdge* ধ্রুব edgep : bbp->inEdges().unlinkable()) edgep->relink_শীর্ষ(succp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(bbp->unlinkDelete(এতত্), bbp);
    }

    // Combine sequential blocks
    প্রতি (V3GraphVertex* ধ্রুব vtxp : vertices().unlinkable()) {
        CfgBlock* ধ্রুব srcp = static_cast<CfgBlock*>(vtxp);
        যদি (srcp->isExit()) অনুবর্ত;
        যদি (srcp->isBranch()) অনুবর্ত;
        CfgBlock* ধ্রুব dstp = srcp->takenp();
        যদি (dstp->isJoin()) অনুবর্ত;
        // Combine them
        যদি (srcp->isEnter()) m_enterp = dstp;
        std::vector<Ast_গ্রন্থি_Stmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.size() + dstp->m_stmtps.size());
        stmtps.ঢোকাও(stmtps.end(), dstp->m_stmtps.begin(), dstp->m_stmtps.end());
        dstp->m_stmtps = std::move(stmtps);
        প্রতি (V3GraphEdge* ধ্রুব edgep : srcp->inEdges().unlinkable()) edgep->relink_শীর্ষ(dstp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(srcp->unlinkDelete(এতত্), srcp);
    }

    যদি (m_nEdits != m_nLastOrdered) rpoBlocks();
    যদি (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

শূন্য CfgGraph::breakCriticalEdges() {
    // Gather critical edges
    std::vector<CfgEdge*> criticalEdges;
    criticalEdges.reserve(m_nEdges);
    প্রতি (V3GraphVertex& vtx : vertices()) {
        ধ্রুব CfgBlock& bb = static_cast<ধ্রুব CfgBlock&>(vtx);
        যদি (!bb.isBranch()) অনুবর্ত;
        প্রতি (V3GraphEdge& edge : vtx.outEdges()) {
            ধ্রুব CfgBlock& succ = static_cast<ধ্রুব CfgBlock&>(*edge.শীর্ষ());
            যদি (!succ.isJoin()) অনুবর্ত;
            criticalEdges.emplace_back(static_cast<CfgEdge*>(&edge));
        }
    }
    // Insert blocks
    প্রতি (CfgEdge* ধ্রুব edgep : criticalEdges) {
        CfgBlock* ধ্রুব newp = addBlock();
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
    ধ্রুব auto chaseEdge = [&](CfgEdge* edgep) -> CfgEdge* {
        যাবত্ (সত্য) {
            CfgBlock* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            যদি (dstp->isJoin()) প্রত্যাগম edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            যাবত্ (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                যদি (!dstp) প্রত্যাগম nullptr;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->takenEdgep();
        }
    };

    // Walk down both paths
    CfgEdge* ধ্রুব takenEdgep = chaseEdge(bbp->takenEdgep());
    যদি (!takenEdgep) প্রত্যাগম nullptr;
    CfgEdge* ধ্রুব untknEdgep = chaseEdge(bbp->untknEdgep());
    যদি (!untknEdgep) প্রত্যাগম nullptr;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    যদি (takenEdgep->dstp() != untknEdgep->dstp()) প্রত্যাগম nullptr;

    // Pick up the common successor
    CfgBlock* ধ্রুব succp = takenEdgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    যদি (succp->isTwoWayJoin()) প্রত্যাগম succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    CfgBlock* ধ্রুব joinp = addBlock();
    addTakenEdge(joinp, succp);
    takenEdgep->relink_শীর্ষ(joinp);
    untknEdgep->relink_শীর্ষ(joinp);
    প্রত্যাগম joinp;
}

তার্কিক CfgGraph::ঢোকাও_Two_Way_Joins() {
    // Reset marks
    প্রতি (V3GraphVertex& v : vertices()) v.user(0);

    তার্কিক isSeriesParallel = সত্য;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    প্রতি (V3GraphVertex& v : vertices()) {
        CfgBlock& bb = static_cast<CfgBlock&>(v);
        // Skip if already visited
        যদি (bb.user()) অনুবর্ত;
        // Skip if not a branch
        যদি (!bb.isBranch()) অনুবর্ত;
        // Fix it up, record if failed
        যদি (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = অসত্য;
    }

    যদি (m_nEdits != m_nLastOrdered) rpoBlocks();
    যদি (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    প্রত্যাগম isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

ধ্রুব CfgBlock* Cfg_Dominator_বৃক্ষ::intersect(ধ্রুব CfgBlock* ap, ধ্রুব CfgBlock* bp) {
    যাবত্ (ap != bp) {
        যাবত্ (*ap > *bp) ap = m_bb2Idom[*ap];
        যাবত্ (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    প্রত্যাগম ap;
}

Cfg_Dominator_বৃক্ষ::Cfg_Dominator_বৃক্ষ(ধ্রুব CfgGraph& cfg)
    : m_bb2Idom{cfg.makeBlockMap<ধ্রুব CfgBlock*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    প্রতি (তার্কিক changed = সত্য; changed;) {
        changed = অসত্য;
        // For each vertex except enter block
        প্রতি (ধ্রুব V3GraphVertex& vtx : cfg.vertices()) {
            ধ্রুব CfgBlock& curr = static_cast<ধ্রুব CfgBlock&>(vtx);
            যদি (curr.isEnter()) অনুবর্ত;  // Skip entry block

            // For each predecessor of current block
            ধ্রুব CfgBlock* idom = nullptr;
            প্রতি (ধ্রুব V3GraphEdge& edge : curr.inEdges()) {
                ধ্রুব CfgBlock& pred = static_cast<ধ্রুব CfgBlock&>(*edge.fromp());
                // Skip if perdecessor not yet processed
                যদি (!m_bb2Idom[pred]) অনুবর্ত;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            যদি (idom == m_bb2Idom[curr]) অনুবর্ত;
            m_bb2Idom[curr] = idom;
            changed = সত্য;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = nullptr;
}
