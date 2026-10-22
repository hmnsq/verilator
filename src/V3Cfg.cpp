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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Cfg.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3EmitV.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// CfgBlock method definitions

std::string CfgBlock::name() மாறிலி {
    std::stringstream ss;
    ss << "BB " + std::to_string(id()) + ":\n";
    க்கு (Ast_முனை* nodep : m_stmtps) {
        எனில் (மாறிலி AstIf* மாறிலி ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debug_Verilog_For_மரம்(ifp->condp(), ss);
            ss << ") ...";
        } இல்லையெனில் எனில் (மாறிலி AstLoopTest* மாறிலி testp = VN_CAST(nodep, LoopTest)) {
            ss << "if  (!";
            V3EmitV::debug_Verilog_For_மரம்(testp->condp(), ss);
            ss << ") break;";
        } இல்லையெனில் {
            V3EmitV::debug_Verilog_For_மரம்(nodep, ss);
        }
    }
    std::string text = VString::replaceSubstr(
        VString::replaceSubstr(ss.str(), "\n", "\\l        "), "\"", "\\\"");
    எனில் (isEnter()) text = "**ENTER**\n" + text;
    எனில் (isExit()) text = text + "\n**EXIT**";
    திரும்பு text;
}
std::string CfgBlock::dotShape() மாறிலி { திரும்பு "rect"; }
std::string CfgBlock::dotRank() மாறிலி {
    எனில் (isEnter()) திரும்பு "source";
    எனில் (isExit()) திரும்பு "sink";
    திரும்பு "";
}

//######################################################################
// CfgEdge method definitions

std::string CfgEdge::dotLabel() மாறிலி {
    std::string label = "E" + std::to_string(id());
    மாறிலி CfgEdge* மாறிலி untknp = srcp()->untknEdgep();
    எனில் (இது == untknp) {
        label += " / F";
    } இல்லையெனில் எனில் (untknp) {
        label += " / T";
    }
    திரும்பு label;
}

//######################################################################
// CfgGraph method definitions

நிலையான வெறுமை cfgOrderVisitBlock(std::vector<CfgBlock*>& postOrderEnumeration, CfgBlock* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    எனில் (CfgBlock* மாறிலி takenp = bbp->takenp()) {
        எனில் (!takenp->user()) cfgOrderVisitBlock(postOrderEnumeration, takenp);
        எனில் (CfgBlock* மாறிலி untknp = bbp->untknp()) {
            எனில் (!untknp->user()) cfgOrderVisitBlock(postOrderEnumeration, untknp);
        }
    }
    // Add to post order enumeration
    postOrderEnumeration.emplace_back(bbp);
};

வெறுமை CfgGraph::rpoBlocks() {
    UASSERT_OBJ(m_nEdits != m_nLastOrdered, m_enterp, "Redundant 'CfgGraph::order' call");
    m_nLastOrdered = m_nEdits;

    // Reset marks
    க்கு (V3GraphVertex& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::vector<CfgBlock*> postOrderEnumeration;
    postOrderEnumeration.reserve(m_nBlocks);
    cfgOrderVisitBlock(postOrderEnumeration, m_enterp);
    UASSERT_OBJ(postOrderEnumeration.size() == m_nBlocks, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    க்கு (size_t i = 0; i < postOrderEnumeration.size(); ++i) {
        CfgBlock* மாறிலி bbp = postOrderEnumeration[m_nBlocks - 1 - i];
        bbp->m_rpo_எண் = i;
        vertices().unlink(bbp);
        vertices().linkBack(bbp);
    }

    // Assign edge IDs
    size_t edge_மொத்தம் = 0;
    க்கு (V3GraphVertex& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        க்கு (V3GraphEdge& e : v.outEdges()) static_cast<CfgEdge&>(e).m_id = edge_மொத்தம்++;
    }
    UASSERT_OBJ(edge_மொத்தம் == m_nEdges, m_enterp, "Inconsistent edge count");
}

தருக்க CfgGraph::containsLoop() மாறிலி {
    க்கு (மாறிலி V3GraphVertex& vtx : vertices()) {
        மாறிலி CfgBlock& current = static_cast<மாறிலி CfgBlock&>(vtx);
        க்கு (மாறிலி V3GraphEdge& edge : current.outEdges()) {
            மாறிலி CfgBlock& successor = *static_cast<மாறிலி CfgBlock*>(edge.உச்சி());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            எனில் (successor.id() < current.id()) திரும்பு உண்மை;
        }
    }
    திரும்பு பொய்;
}

வெறுமை CfgGraph::minimize() {
    // Remove empty blocks (except enter and exit)
    க்கு (V3GraphVertex* மாறிலி vtxp : vertices().unlinkable()) {
        CfgBlock* மாறிலி bbp = static_cast<CfgBlock*>(vtxp);
        எனில் (bbp->isEnter()) தொடர்;
        எனில் (bbp->isExit()) தொடர்;
        எனில் (!bbp->stmtps().empty()) தொடர்;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        CfgBlock* மாறிலி succp = bbp->takenp();
        க்கு (V3GraphEdge* மாறிலி edgep : bbp->inEdges().unlinkable()) edgep->relink_உச்சி(succp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(bbp->unlinkDelete(இது), bbp);
    }

    // Combine sequential blocks
    க்கு (V3GraphVertex* மாறிலி vtxp : vertices().unlinkable()) {
        CfgBlock* மாறிலி srcp = static_cast<CfgBlock*>(vtxp);
        எனில் (srcp->isExit()) தொடர்;
        எனில் (srcp->isBranch()) தொடர்;
        CfgBlock* மாறிலி dstp = srcp->takenp();
        எனில் (dstp->isJoin()) தொடர்;
        // Combine them
        எனில் (srcp->isEnter()) m_enterp = dstp;
        std::vector<Ast_முனை_Stmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.size() + dstp->m_stmtps.size());
        stmtps.செருகு(stmtps.end(), dstp->m_stmtps.begin(), dstp->m_stmtps.end());
        dstp->m_stmtps = std::move(stmtps);
        க்கு (V3GraphEdge* மாறிலி edgep : srcp->inEdges().unlinkable()) edgep->relink_உச்சி(dstp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(srcp->unlinkDelete(இது), srcp);
    }

    எனில் (m_nEdits != m_nLastOrdered) rpoBlocks();
    எனில் (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

வெறுமை CfgGraph::breakCriticalEdges() {
    // Gather critical edges
    std::vector<CfgEdge*> criticalEdges;
    criticalEdges.reserve(m_nEdges);
    க்கு (V3GraphVertex& vtx : vertices()) {
        மாறிலி CfgBlock& bb = static_cast<மாறிலி CfgBlock&>(vtx);
        எனில் (!bb.isBranch()) தொடர்;
        க்கு (V3GraphEdge& edge : vtx.outEdges()) {
            மாறிலி CfgBlock& succ = static_cast<மாறிலி CfgBlock&>(*edge.உச்சி());
            எனில் (!succ.isJoin()) தொடர்;
            criticalEdges.emplace_back(static_cast<CfgEdge*>(&edge));
        }
    }
    // Insert blocks
    க்கு (CfgEdge* மாறிலி edgep : criticalEdges) {
        CfgBlock* மாறிலி newp = addBlock();
        addTakenEdge(newp, edgep->dstp());
        edgep->relink_உச்சி(newp);
    }

    எனில் (m_nEdits != m_nLastOrdered) rpoBlocks();
    எனில் (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-breakCriticalEdges");
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
    மாறிலி auto chaseEdge = [&](CfgEdge* edgep) -> CfgEdge* {
        போது (உண்மை) {
            CfgBlock* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            எனில் (dstp->isJoin()) திரும்பு edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            போது (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                எனில் (!dstp) திரும்பு nullptr;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->takenEdgep();
        }
    };

    // Walk down both paths
    CfgEdge* மாறிலி takenEdgep = chaseEdge(bbp->takenEdgep());
    எனில் (!takenEdgep) திரும்பு nullptr;
    CfgEdge* மாறிலி untknEdgep = chaseEdge(bbp->untknEdgep());
    எனில் (!untknEdgep) திரும்பு nullptr;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    எனில் (takenEdgep->dstp() != untknEdgep->dstp()) திரும்பு nullptr;

    // Pick up the common successor
    CfgBlock* மாறிலி succp = takenEdgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    எனில் (succp->isTwoWayJoin()) திரும்பு succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    CfgBlock* மாறிலி joinp = addBlock();
    addTakenEdge(joinp, succp);
    takenEdgep->relink_உச்சி(joinp);
    untknEdgep->relink_உச்சி(joinp);
    திரும்பு joinp;
}

தருக்க CfgGraph::செருகு_Two_Way_Joins() {
    // Reset marks
    க்கு (V3GraphVertex& v : vertices()) v.user(0);

    தருக்க isSeriesParallel = உண்மை;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    க்கு (V3GraphVertex& v : vertices()) {
        CfgBlock& bb = static_cast<CfgBlock&>(v);
        // Skip if already visited
        எனில் (bb.user()) தொடர்;
        // Skip if not a branch
        எனில் (!bb.isBranch()) தொடர்;
        // Fix it up, record if failed
        எனில் (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = பொய்;
    }

    எனில் (m_nEdits != m_nLastOrdered) rpoBlocks();
    எனில் (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    திரும்பு isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

மாறிலி CfgBlock* Cfg_Dominator_மரம்::intersect(மாறிலி CfgBlock* ap, மாறிலி CfgBlock* bp) {
    போது (ap != bp) {
        போது (*ap > *bp) ap = m_bb2Idom[*ap];
        போது (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    திரும்பு ap;
}

Cfg_Dominator_மரம்::Cfg_Dominator_மரம்(மாறிலி CfgGraph& cfg)
    : m_bb2Idom{cfg.makeBlockMap<மாறிலி CfgBlock*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    க்கு (தருக்க changed = உண்மை; changed;) {
        changed = பொய்;
        // For each vertex except enter block
        க்கு (மாறிலி V3GraphVertex& vtx : cfg.vertices()) {
            மாறிலி CfgBlock& curr = static_cast<மாறிலி CfgBlock&>(vtx);
            எனில் (curr.isEnter()) தொடர்;  // Skip entry block

            // For each predecessor of current block
            மாறிலி CfgBlock* idom = nullptr;
            க்கு (மாறிலி V3GraphEdge& edge : curr.inEdges()) {
                மாறிலி CfgBlock& pred = static_cast<மாறிலி CfgBlock&>(*edge.fromp());
                // Skip if perdecessor not yet processed
                எனில் (!m_bb2Idom[pred]) தொடர்;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            எனில் (idom == m_bb2Idom[curr]) தொடர்;
            m_bb2Idom[curr] = idom;
            changed = உண்மை;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = nullptr;
}
