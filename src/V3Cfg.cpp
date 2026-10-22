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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Cfg.h"

#ضمن "V3Ast.h"
#ضمن "V3EmitV.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// CfgBlock method definitions

std::string CfgBlock::name() ثابت {
    std::stringstream ss;
    ss << "BB " + std::to_string(id()) + ":\n";
    لكل (AstNode* nodep : m_stmtps) {
        𐤀𐤌 (ثابت AstIf* ثابت ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debugVerilogForTree(ifp->condp(), ss);
            ss << ") ...";
        } وإلا 𐤀𐤌 (ثابت AstLoopTest* ثابت testp = VN_CAST(nodep, LoopTest)) {
            ss << "if  (!";
            V3EmitV::debugVerilogForTree(testp->condp(), ss);
            ss << ") break;";
        } وإلا {
            V3EmitV::debugVerilogForTree(nodep, ss);
        }
    }
    std::string text = VString::replaceSubstr(
        VString::replaceSubstr(ss.str(), "\n", "\\l        "), "\"", "\\\"");
    𐤀𐤌 (isEnter()) text = "**ENTER**\n" + text;
    𐤀𐤌 (isExit()) text = text + "\n**EXIT**";
    أرجع text;
}
std::string CfgBlock::dotShape() ثابت { أرجع "rect"; }
std::string CfgBlock::dotRank() ثابت {
    𐤀𐤌 (isEnter()) أرجع "source";
    𐤀𐤌 (isExit()) أرجع "sink";
    أرجع "";
}

//######################################################################
// CfgEdge method definitions

std::string CfgEdge::dotLabel() ثابت {
    std::string label = "E" + std::to_string(id());
    ثابت CfgEdge* ثابت untknp = srcp()->untknEdgep();
    𐤀𐤌 (هذا == untknp) {
        label += " / F";
    } وإلا 𐤀𐤌 (untknp) {
        label += " / T";
    }
    أرجع label;
}

//######################################################################
// CfgGraph method definitions

ساكن فراغ cfgOrderVisitBlock(std::vector<CfgBlock*>& postOrderEnumeration, CfgBlock* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    𐤀𐤌 (CfgBlock* ثابت takenp = bbp->takenp()) {
        𐤀𐤌 (!takenp->user()) cfgOrderVisitBlock(postOrderEnumeration, takenp);
        𐤀𐤌 (CfgBlock* ثابت untknp = bbp->untknp()) {
            𐤀𐤌 (!untknp->user()) cfgOrderVisitBlock(postOrderEnumeration, untknp);
        }
    }
    // Add to post order enumeration
    postOrderEnumeration.emplace_back(bbp);
};

فراغ CfgGraph::rpoBlocks() {
    UASSERT_OBJ(m_nEdits != m_nLastOrdered, m_enterp, "Redundant 'CfgGraph::order' call");
    m_nLastOrdered = m_nEdits;

    // Reset marks
    لكل (V3GraphVertex& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::vector<CfgBlock*> postOrderEnumeration;
    postOrderEnumeration.reserve(m_nBlocks);
    cfgOrderVisitBlock(postOrderEnumeration, m_enterp);
    UASSERT_OBJ(postOrderEnumeration.size() == m_nBlocks, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    لكل (size_t i = 0; i < postOrderEnumeration.size(); ++i) {
        CfgBlock* ثابت bbp = postOrderEnumeration[m_nBlocks - 1 - i];
        bbp->m_rpoNumber = i;
        vertices().unlink(bbp);
        vertices().linkBack(bbp);
    }

    // Assign edge IDs
    size_t edgeCount = 0;
    لكل (V3GraphVertex& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        لكل (V3GraphEdge& e : v.outEdges()) static_cast<CfgEdge&>(e).m_id = edgeCount++;
    }
    UASSERT_OBJ(edgeCount == m_nEdges, m_enterp, "Inconsistent edge count");
}

منطقي CfgGraph::containsLoop() ثابت {
    لكل (ثابت V3GraphVertex& vtx : vertices()) {
        ثابت CfgBlock& current = static_cast<ثابت CfgBlock&>(vtx);
        لكل (ثابت V3GraphEdge& edge : current.outEdges()) {
            ثابت CfgBlock& successor = *static_cast<ثابت CfgBlock*>(edge.top());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            𐤀𐤌 (successor.id() < current.id()) أرجع true;
        }
    }
    أرجع خطأ;
}

فراغ CfgGraph::minimize() {
    // Remove empty blocks (except enter and exit)
    لكل (V3GraphVertex* ثابت vtxp : vertices().unlinkable()) {
        CfgBlock* ثابت bbp = static_cast<CfgBlock*>(vtxp);
        𐤀𐤌 (bbp->isEnter()) استمر;
        𐤀𐤌 (bbp->isExit()) استمر;
        𐤀𐤌 (!bbp->stmtps().empty()) استمر;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        CfgBlock* ثابت succp = bbp->takenp();
        لكل (V3GraphEdge* ثابت edgep : bbp->inEdges().unlinkable()) edgep->relinkTop(succp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(bbp->unlinkDelete(هذا), bbp);
    }

    // Combine sequential blocks
    لكل (V3GraphVertex* ثابت vtxp : vertices().unlinkable()) {
        CfgBlock* ثابت srcp = static_cast<CfgBlock*>(vtxp);
        𐤀𐤌 (srcp->isExit()) استمر;
        𐤀𐤌 (srcp->isBranch()) استمر;
        CfgBlock* ثابت dstp = srcp->takenp();
        𐤀𐤌 (dstp->isJoin()) استمر;
        // Combine them
        𐤀𐤌 (srcp->isEnter()) m_enterp = dstp;
        std::vector<AstNodeStmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.size() + dstp->m_stmtps.size());
        stmtps.insert(stmtps.end(), dstp->m_stmtps.begin(), dstp->m_stmtps.end());
        dstp->m_stmtps = std::move(stmtps);
        لكل (V3GraphEdge* ثابت edgep : srcp->inEdges().unlinkable()) edgep->relinkTop(dstp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(srcp->unlinkDelete(هذا), srcp);
    }

    𐤀𐤌 (m_nEdits != m_nLastOrdered) rpoBlocks();
    𐤀𐤌 (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

فراغ CfgGraph::breakCriticalEdges() {
    // Gather critical edges
    std::vector<CfgEdge*> criticalEdges;
    criticalEdges.reserve(m_nEdges);
    لكل (V3GraphVertex& vtx : vertices()) {
        ثابت CfgBlock& bb = static_cast<ثابت CfgBlock&>(vtx);
        𐤀𐤌 (!bb.isBranch()) استمر;
        لكل (V3GraphEdge& edge : vtx.outEdges()) {
            ثابت CfgBlock& succ = static_cast<ثابت CfgBlock&>(*edge.top());
            𐤀𐤌 (!succ.isJoin()) استمر;
            criticalEdges.emplace_back(static_cast<CfgEdge*>(&edge));
        }
    }
    // Insert blocks
    لكل (CfgEdge* ثابت edgep : criticalEdges) {
        CfgBlock* ثابت newp = addBlock();
        addTakenEdge(newp, edgep->dstp());
        edgep->relinkTop(newp);
    }

    𐤀𐤌 (m_nEdits != m_nLastOrdered) rpoBlocks();
    𐤀𐤌 (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-breakCriticalEdges");
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
        طالما (true) {
            CfgBlock* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            𐤀𐤌 (dstp->isJoin()) أرجع edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            طالما (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                𐤀𐤌 (!dstp) أرجع nullptr;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->takenEdgep();
        }
    };

    // Walk down both paths
    CfgEdge* ثابت takenEdgep = chaseEdge(bbp->takenEdgep());
    𐤀𐤌 (!takenEdgep) أرجع nullptr;
    CfgEdge* ثابت untknEdgep = chaseEdge(bbp->untknEdgep());
    𐤀𐤌 (!untknEdgep) أرجع nullptr;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    𐤀𐤌 (takenEdgep->dstp() != untknEdgep->dstp()) أرجع nullptr;

    // Pick up the common successor
    CfgBlock* ثابت succp = takenEdgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    𐤀𐤌 (succp->isTwoWayJoin()) أرجع succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    CfgBlock* ثابت joinp = addBlock();
    addTakenEdge(joinp, succp);
    takenEdgep->relinkTop(joinp);
    untknEdgep->relinkTop(joinp);
    أرجع joinp;
}

منطقي CfgGraph::insertTwoWayJoins() {
    // Reset marks
    لكل (V3GraphVertex& v : vertices()) v.user(0);

    منطقي isSeriesParallel = true;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    لكل (V3GraphVertex& v : vertices()) {
        CfgBlock& bb = static_cast<CfgBlock&>(v);
        // Skip if already visited
        𐤀𐤌 (bb.user()) استمر;
        // Skip if not a branch
        𐤀𐤌 (!bb.isBranch()) استمر;
        // Fix it up, record if failed
        𐤀𐤌 (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = خطأ;
    }

    𐤀𐤌 (m_nEdits != m_nLastOrdered) rpoBlocks();
    𐤀𐤌 (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    أرجع isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

ثابت CfgBlock* CfgDominatorTree::intersect(ثابت CfgBlock* ap, ثابت CfgBlock* bp) {
    طالما (ap != bp) {
        طالما (*ap > *bp) ap = m_bb2Idom[*ap];
        طالما (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    أرجع ap;
}

CfgDominatorTree::CfgDominatorTree(ثابت CfgGraph& cfg)
    : m_bb2Idom{cfg.makeBlockMap<ثابت CfgBlock*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    لكل (منطقي changed = true; changed;) {
        changed = خطأ;
        // For each vertex except enter block
        لكل (ثابت V3GraphVertex& vtx : cfg.vertices()) {
            ثابت CfgBlock& curr = static_cast<ثابت CfgBlock&>(vtx);
            𐤀𐤌 (curr.isEnter()) استمر;  // Skip entry block

            // For each predecessor of current block
            ثابت CfgBlock* idom = nullptr;
            لكل (ثابت V3GraphEdge& edge : curr.inEdges()) {
                ثابت CfgBlock& pred = static_cast<ثابت CfgBlock&>(*edge.fromp());
                // Skip if perdecessor not yet processed
                𐤀𐤌 (!m_bb2Idom[pred]) استمر;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            𐤀𐤌 (idom == m_bb2Idom[curr]) استمر;
            m_bb2Idom[curr] = idom;
            changed = true;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = nullptr;
}
