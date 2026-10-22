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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Cfg.h"

#شامل "V3Ast.h"
#شامل "V3EmitV.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// CfgBlock method definitions

std::string CfgBlock::name() ثابت {
    std::stringstream ss;
    ss << "BB " + std::to_string(id()) + ":\n";
    برای (Ast_گره* nodep : m_stmtps) {
        اگر (ثابت AstIf* ثابت ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debug_Verilog_For_درخت(ifp->condp(), ss);
            ss << ") ...";
        } وگرنه اگر (ثابت AstLoopTest* ثابت testp = VN_CAST(nodep, LoopTest)) {
            ss << "if  (!";
            V3EmitV::debug_Verilog_For_درخت(testp->condp(), ss);
            ss << ") break;";
        } وگرنه {
            V3EmitV::debug_Verilog_For_درخت(nodep, ss);
        }
    }
    std::string text = VString::replaceSubstr(
        VString::replaceSubstr(ss.str(), "\n", "\\l        "), "\"", "\\\"");
    اگر (isEnter()) text = "**ENTER**\n" + text;
    اگر (isExit()) text = text + "\n**EXIT**";
    بازگشت text;
}
std::string CfgBlock::dotShape() ثابت { بازگشت "rect"; }
std::string CfgBlock::dotRank() ثابت {
    اگر (isEnter()) بازگشت "source";
    اگر (isExit()) بازگشت "sink";
    بازگشت "";
}

//######################################################################
// CfgEdge method definitions

std::string CfgEdge::dotLabel() ثابت {
    std::string label = "E" + std::to_string(id());
    ثابت CfgEdge* ثابت untknp = srcp()->untknEdgep();
    اگر (این == untknp) {
        label += " / F";
    } وگرنه اگر (untknp) {
        label += " / T";
    }
    بازگشت label;
}

//######################################################################
// CfgGraph method definitions

ایستا تهی cfgOrderVisitBlock(std::vector<CfgBlock*>& postOrderEnumeration, CfgBlock* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    اگر (CfgBlock* ثابت takenp = bbp->takenp()) {
        اگر (!takenp->user()) cfgOrderVisitBlock(postOrderEnumeration, takenp);
        اگر (CfgBlock* ثابت untknp = bbp->untknp()) {
            اگر (!untknp->user()) cfgOrderVisitBlock(postOrderEnumeration, untknp);
        }
    }
    // Add to post order enumeration
    postOrderEnumeration.emplace_back(bbp);
};

تهی CfgGraph::rpoBlocks() {
    UASSERT_OBJ(m_nEdits != m_nLastOrdered, m_enterp, "Redundant 'CfgGraph::order' call");
    m_nLastOrdered = m_nEdits;

    // Reset marks
    برای (V3GraphVertex& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::vector<CfgBlock*> postOrderEnumeration;
    postOrderEnumeration.reserve(m_nBlocks);
    cfgOrderVisitBlock(postOrderEnumeration, m_enterp);
    UASSERT_OBJ(postOrderEnumeration.size() == m_nBlocks, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    برای (size_t i = 0; i < postOrderEnumeration.size(); ++i) {
        CfgBlock* ثابت bbp = postOrderEnumeration[m_nBlocks - 1 - i];
        bbp->m_rpo_عدد = i;
        vertices().unlink(bbp);
        vertices().linkBack(bbp);
    }

    // Assign edge IDs
    size_t edge_شمار = 0;
    برای (V3GraphVertex& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        برای (V3GraphEdge& e : v.outEdges()) static_cast<CfgEdge&>(e).m_id = edge_شمار++;
    }
    UASSERT_OBJ(edge_شمار == m_nEdges, m_enterp, "Inconsistent edge count");
}

منطقی CfgGraph::containsLoop() ثابت {
    برای (ثابت V3GraphVertex& vtx : vertices()) {
        ثابت CfgBlock& current = static_cast<ثابت CfgBlock&>(vtx);
        برای (ثابت V3GraphEdge& edge : current.outEdges()) {
            ثابت CfgBlock& successor = *static_cast<ثابت CfgBlock*>(edge.بالاترین());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            اگر (successor.id() < current.id()) بازگشت درست;
        }
    }
    بازگشت نادرست;
}

تهی CfgGraph::minimize() {
    // Remove empty blocks (except enter and exit)
    برای (V3GraphVertex* ثابت vtxp : vertices().unlinkable()) {
        CfgBlock* ثابت bbp = static_cast<CfgBlock*>(vtxp);
        اگر (bbp->isEnter()) ادامه;
        اگر (bbp->isExit()) ادامه;
        اگر (!bbp->stmtps().empty()) ادامه;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        CfgBlock* ثابت succp = bbp->takenp();
        برای (V3GraphEdge* ثابت edgep : bbp->inEdges().unlinkable()) edgep->relink_بالاترین(succp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(bbp->unlinkDelete(این), bbp);
    }

    // Combine sequential blocks
    برای (V3GraphVertex* ثابت vtxp : vertices().unlinkable()) {
        CfgBlock* ثابت srcp = static_cast<CfgBlock*>(vtxp);
        اگر (srcp->isExit()) ادامه;
        اگر (srcp->isBranch()) ادامه;
        CfgBlock* ثابت dstp = srcp->takenp();
        اگر (dstp->isJoin()) ادامه;
        // Combine them
        اگر (srcp->isEnter()) m_enterp = dstp;
        std::vector<Ast_گره_Stmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.size() + dstp->m_stmtps.size());
        stmtps.درج(stmtps.end(), dstp->m_stmtps.begin(), dstp->m_stmtps.end());
        dstp->m_stmtps = std::move(stmtps);
        برای (V3GraphEdge* ثابت edgep : srcp->inEdges().unlinkable()) edgep->relink_بالاترین(dstp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(srcp->unlinkDelete(این), srcp);
    }

    اگر (m_nEdits != m_nLastOrdered) rpoBlocks();
    اگر (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

تهی CfgGraph::breakCriticalEdges() {
    // Gather critical edges
    std::vector<CfgEdge*> criticalEdges;
    criticalEdges.reserve(m_nEdges);
    برای (V3GraphVertex& vtx : vertices()) {
        ثابت CfgBlock& bb = static_cast<ثابت CfgBlock&>(vtx);
        اگر (!bb.isBranch()) ادامه;
        برای (V3GraphEdge& edge : vtx.outEdges()) {
            ثابت CfgBlock& succ = static_cast<ثابت CfgBlock&>(*edge.بالاترین());
            اگر (!succ.isJoin()) ادامه;
            criticalEdges.emplace_back(static_cast<CfgEdge*>(&edge));
        }
    }
    // Insert blocks
    برای (CfgEdge* ثابت edgep : criticalEdges) {
        CfgBlock* ثابت newp = addBlock();
        addTakenEdge(newp, edgep->dstp());
        edgep->relink_بالاترین(newp);
    }

    اگر (m_nEdits != m_nLastOrdered) rpoBlocks();
    اگر (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-breakCriticalEdges");
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
    ثابت auto chaseEdge = [&](CfgEdge* edgep) -> CfgEdge* {
        وقتی (درست) {
            CfgBlock* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            اگر (dstp->isJoin()) بازگشت edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            وقتی (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                اگر (!dstp) بازگشت nullptr;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->takenEdgep();
        }
    };

    // Walk down both paths
    CfgEdge* ثابت takenEdgep = chaseEdge(bbp->takenEdgep());
    اگر (!takenEdgep) بازگشت nullptr;
    CfgEdge* ثابت untknEdgep = chaseEdge(bbp->untknEdgep());
    اگر (!untknEdgep) بازگشت nullptr;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    اگر (takenEdgep->dstp() != untknEdgep->dstp()) بازگشت nullptr;

    // Pick up the common successor
    CfgBlock* ثابت succp = takenEdgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    اگر (succp->isTwoWayJoin()) بازگشت succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    CfgBlock* ثابت joinp = addBlock();
    addTakenEdge(joinp, succp);
    takenEdgep->relink_بالاترین(joinp);
    untknEdgep->relink_بالاترین(joinp);
    بازگشت joinp;
}

منطقی CfgGraph::درج_Two_Way_Joins() {
    // Reset marks
    برای (V3GraphVertex& v : vertices()) v.user(0);

    منطقی isSeriesParallel = درست;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    برای (V3GraphVertex& v : vertices()) {
        CfgBlock& bb = static_cast<CfgBlock&>(v);
        // Skip if already visited
        اگر (bb.user()) ادامه;
        // Skip if not a branch
        اگر (!bb.isBranch()) ادامه;
        // Fix it up, record if failed
        اگر (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = نادرست;
    }

    اگر (m_nEdits != m_nLastOrdered) rpoBlocks();
    اگر (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    بازگشت isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

ثابت CfgBlock* Cfg_Dominator_درخت::intersect(ثابت CfgBlock* ap, ثابت CfgBlock* bp) {
    وقتی (ap != bp) {
        وقتی (*ap > *bp) ap = m_bb2Idom[*ap];
        وقتی (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    بازگشت ap;
}

Cfg_Dominator_درخت::Cfg_Dominator_درخت(ثابت CfgGraph& cfg)
    : m_bb2Idom{cfg.makeBlockMap<ثابت CfgBlock*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    برای (منطقی changed = درست; changed;) {
        changed = نادرست;
        // For each vertex except enter block
        برای (ثابت V3GraphVertex& vtx : cfg.vertices()) {
            ثابت CfgBlock& curr = static_cast<ثابت CfgBlock&>(vtx);
            اگر (curr.isEnter()) ادامه;  // Skip entry block

            // For each predecessor of current block
            ثابت CfgBlock* idom = nullptr;
            برای (ثابت V3GraphEdge& edge : curr.inEdges()) {
                ثابت CfgBlock& pred = static_cast<ثابت CfgBlock&>(*edge.fromp());
                // Skip if perdecessor not yet processed
                اگر (!m_bb2Idom[pred]) ادامه;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            اگر (idom == m_bb2Idom[curr]) ادامه;
            m_bb2Idom[curr] = idom;
            changed = درست;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = nullptr;
}
