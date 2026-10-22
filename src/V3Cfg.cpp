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

#include "config_build.h"
#include "verilatedos.h"

#include "V3Cfg.h"

#include "V3Ast.h"
#include "V3EmitV.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// CfgBlock method definitions

std::string CfgBlock::name() const {
    std::stringstream ss;
    ss << "BB " + std::to_string(id()) + ":\n";
    དོན་ལུ (AstNode* nodep : m_stmtps) {
        གལ་སྲིད (const AstIf* const ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debugVerilogForTree(ifp->condp(), ss);
            ss << ") ...";
        } དེ་མིན གལ་སྲིད (const AstLoopTest* const testp = VN_CAST(nodep, LoopTest)) {
            ss << "if  (!";
            V3EmitV::debugVerilogForTree(testp->condp(), ss);
            ss << ") break;";
        } དེ་མིན {
            V3EmitV::debugVerilogForTree(nodep, ss);
        }
    }
    std::string text = VString::replaceSubstr(
        VString::replaceSubstr(ss.str(), "\n", "\\l        "), "\"", "\\\"");
    གལ་སྲིད (isEnter()) text = "**ENTER**\n" + text;
    གལ་སྲིད (isExit()) text = text + "\n**EXIT**";
    ལོག text;
}
std::string CfgBlock::dotShape() const { ལོག "rect"; }
std::string CfgBlock::dotRank() const {
    གལ་སྲིད (isEnter()) ལོག "source";
    གལ་སྲིད (isExit()) ལོག "sink";
    ལོག "";
}

//######################################################################
// CfgEdge method definitions

std::string CfgEdge::dotLabel() const {
    std::string label = "E" + std::to_string(id());
    const CfgEdge* const untknp = srcp()->untknEdgep();
    གལ་སྲིད (this == untknp) {
        label += " / F";
    } དེ་མིན གལ་སྲིད (untknp) {
        label += " / T";
    }
    ལོག label;
}

//######################################################################
// CfgGraph method definitions

static void cfgOrderVisitBlock(std::vector<CfgBlock*>& postOrderEnumeration, CfgBlock* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    གལ་སྲིད (CfgBlock* const takenp = bbp->takenp()) {
        གལ་སྲིད (!takenp->user()) cfgOrderVisitBlock(postOrderEnumeration, takenp);
        གལ་སྲིད (CfgBlock* const untknp = bbp->untknp()) {
            གལ་སྲིད (!untknp->user()) cfgOrderVisitBlock(postOrderEnumeration, untknp);
        }
    }
    // Add to post order enumeration
    postOrderEnumeration.emplace_back(bbp);
};

void CfgGraph::rpoBlocks() {
    UASSERT_OBJ(m_nEdits != m_nLastOrdered, m_enterp, "Redundant 'CfgGraph::order' call");
    m_nLastOrdered = m_nEdits;

    // Reset marks
    དོན་ལུ (V3GraphVertex& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::vector<CfgBlock*> postOrderEnumeration;
    postOrderEnumeration.reserve(m_nBlocks);
    cfgOrderVisitBlock(postOrderEnumeration, m_enterp);
    UASSERT_OBJ(postOrderEnumeration.size() == m_nBlocks, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    དོན་ལུ (size_t i = 0; i < postOrderEnumeration.size(); ++i) {
        CfgBlock* const bbp = postOrderEnumeration[m_nBlocks - 1 - i];
        bbp->m_rpoNumber = i;
        vertices().unlink(bbp);
        vertices().linkBack(bbp);
    }

    // Assign edge IDs
    size_t edgeCount = 0;
    དོན་ལུ (V3GraphVertex& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        དོན་ལུ (V3GraphEdge& e : v.outEdges()) static_cast<CfgEdge&>(e).m_id = edgeCount++;
    }
    UASSERT_OBJ(edgeCount == m_nEdges, m_enterp, "Inconsistent edge count");
}

bool CfgGraph::containsLoop() const {
    དོན་ལུ (const V3GraphVertex& vtx : vertices()) {
        const CfgBlock& current = static_cast<const CfgBlock&>(vtx);
        དོན་ལུ (const V3GraphEdge& edge : current.outEdges()) {
            const CfgBlock& successor = *static_cast<const CfgBlock*>(edge.top());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            གལ་སྲིད (successor.id() < current.id()) ལོག true;
        }
    }
    ལོག false;
}

void CfgGraph::minimize() {
    // Remove empty blocks (except enter and exit)
    དོན་ལུ (V3GraphVertex* const vtxp : vertices().unlinkable()) {
        CfgBlock* const bbp = static_cast<CfgBlock*>(vtxp);
        གལ་སྲིད (bbp->isEnter()) continue;
        གལ་སྲིད (bbp->isExit()) continue;
        གལ་སྲིད (!bbp->stmtps().empty()) continue;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        CfgBlock* const succp = bbp->takenp();
        དོན་ལུ (V3GraphEdge* const edgep : bbp->inEdges().unlinkable()) edgep->relinkTop(succp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(bbp->unlinkDelete(this), bbp);
    }

    // Combine sequential blocks
    དོན་ལུ (V3GraphVertex* const vtxp : vertices().unlinkable()) {
        CfgBlock* const srcp = static_cast<CfgBlock*>(vtxp);
        གལ་སྲིད (srcp->isExit()) continue;
        གལ་སྲིད (srcp->isBranch()) continue;
        CfgBlock* const dstp = srcp->takenp();
        གལ་སྲིད (dstp->isJoin()) continue;
        // Combine them
        གལ་སྲིད (srcp->isEnter()) m_enterp = dstp;
        std::vector<AstNodeStmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.size() + dstp->m_stmtps.size());
        stmtps.insert(stmtps.end(), dstp->m_stmtps.begin(), dstp->m_stmtps.end());
        dstp->m_stmtps = std::move(stmtps);
        དོན་ལུ (V3GraphEdge* const edgep : srcp->inEdges().unlinkable()) edgep->relinkTop(dstp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(srcp->unlinkDelete(this), srcp);
    }

    གལ་སྲིད (m_nEdits != m_nLastOrdered) rpoBlocks();
    གལ་སྲིད (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

void CfgGraph::breakCriticalEdges() {
    // Gather critical edges
    std::vector<CfgEdge*> criticalEdges;
    criticalEdges.reserve(m_nEdges);
    དོན་ལུ (V3GraphVertex& vtx : vertices()) {
        const CfgBlock& bb = static_cast<const CfgBlock&>(vtx);
        གལ་སྲིད (!bb.isBranch()) continue;
        དོན་ལུ (V3GraphEdge& edge : vtx.outEdges()) {
            const CfgBlock& succ = static_cast<const CfgBlock&>(*edge.top());
            གལ་སྲིད (!succ.isJoin()) continue;
            criticalEdges.emplace_back(static_cast<CfgEdge*>(&edge));
        }
    }
    // Insert blocks
    དོན་ལུ (CfgEdge* const edgep : criticalEdges) {
        CfgBlock* const newp = addBlock();
        addTakenEdge(newp, edgep->dstp());
        edgep->relinkTop(newp);
    }

    གལ་སྲིད (m_nEdits != m_nLastOrdered) rpoBlocks();
    གལ་སྲིད (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-breakCriticalEdges");
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
    const auto chaseEdge = [&](CfgEdge* edgep) -> CfgEdge* {
        སྐབས (true) {
            CfgBlock* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            གལ་སྲིད (dstp->isJoin()) ལོག edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            སྐབས (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                གལ་སྲིད (!dstp) ལོག nullptr;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->takenEdgep();
        }
    };

    // Walk down both paths
    CfgEdge* const takenEdgep = chaseEdge(bbp->takenEdgep());
    གལ་སྲིད (!takenEdgep) ལོག nullptr;
    CfgEdge* const untknEdgep = chaseEdge(bbp->untknEdgep());
    གལ་སྲིད (!untknEdgep) ལོག nullptr;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    གལ་སྲིད (takenEdgep->dstp() != untknEdgep->dstp()) ལོག nullptr;

    // Pick up the common successor
    CfgBlock* const succp = takenEdgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    གལ་སྲིད (succp->isTwoWayJoin()) ལོག succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    CfgBlock* const joinp = addBlock();
    addTakenEdge(joinp, succp);
    takenEdgep->relinkTop(joinp);
    untknEdgep->relinkTop(joinp);
    ལོག joinp;
}

bool CfgGraph::insertTwoWayJoins() {
    // Reset marks
    དོན་ལུ (V3GraphVertex& v : vertices()) v.user(0);

    bool isSeriesParallel = true;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    དོན་ལུ (V3GraphVertex& v : vertices()) {
        CfgBlock& bb = static_cast<CfgBlock&>(v);
        // Skip if already visited
        གལ་སྲིད (bb.user()) continue;
        // Skip if not a branch
        གལ་སྲིད (!bb.isBranch()) continue;
        // Fix it up, record if failed
        གལ་སྲིད (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = false;
    }

    གལ་སྲིད (m_nEdits != m_nLastOrdered) rpoBlocks();
    གལ་སྲིད (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    ལོག isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

const CfgBlock* CfgDominatorTree::intersect(const CfgBlock* ap, const CfgBlock* bp) {
    སྐབས (ap != bp) {
        སྐབས (*ap > *bp) ap = m_bb2Idom[*ap];
        སྐབས (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    ལོག ap;
}

CfgDominatorTree::CfgDominatorTree(const CfgGraph& cfg)
    : m_bb2Idom{cfg.makeBlockMap<const CfgBlock*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    དོན་ལུ (bool changed = true; changed;) {
        changed = false;
        // For each vertex except enter block
        དོན་ལུ (const V3GraphVertex& vtx : cfg.vertices()) {
            const CfgBlock& curr = static_cast<const CfgBlock&>(vtx);
            གལ་སྲིད (curr.isEnter()) continue;  // Skip entry block

            // For each predecessor of current block
            const CfgBlock* idom = nullptr;
            དོན་ལུ (const V3GraphEdge& edge : curr.inEdges()) {
                const CfgBlock& pred = static_cast<const CfgBlock&>(*edge.fromp());
                // Skip if perdecessor not yet processed
                གལ་སྲིད (!m_bb2Idom[pred]) continue;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            གལ་སྲིད (idom == m_bb2Idom[curr]) continue;
            m_bb2Idom[curr] = idom;
            changed = true;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = nullptr;
}
