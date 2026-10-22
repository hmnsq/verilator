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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Cfg.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3EmitV.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// CfgBlock method definitions

std::string CfgBlock::name() અચલ {
    std::stringstream ss;
    ss << "BB " + std::to_string(id()) + ":\n";
    માટે (Ast_ગાંઠ* nodep : m_stmtps) {
        જો (અચલ AstIf* અચલ ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debug_Verilog_For_વૃક્ષ(ifp->condp(), ss);
            ss << ") ...";
        } નહીંતર જો (અચલ AstLoopTest* અચલ testp = VN_CAST(nodep, LoopTest)) {
            ss << "if  (!";
            V3EmitV::debug_Verilog_For_વૃક્ષ(testp->condp(), ss);
            ss << ") break;";
        } નહીંતર {
            V3EmitV::debug_Verilog_For_વૃક્ષ(nodep, ss);
        }
    }
    std::string text = VString::replaceSubstr(
        VString::replaceSubstr(ss.str(), "\n", "\\l        "), "\"", "\\\"");
    જો (isEnter()) text = "**ENTER**\n" + text;
    જો (isExit()) text = text + "\n**EXIT**";
    પાછા text;
}
std::string CfgBlock::dotShape() અચલ { પાછા "rect"; }
std::string CfgBlock::dotRank() અચલ {
    જો (isEnter()) પાછા "source";
    જો (isExit()) પાછા "sink";
    પાછા "";
}

//######################################################################
// CfgEdge method definitions

std::string CfgEdge::dotLabel() અચલ {
    std::string label = "E" + std::to_string(id());
    અચલ CfgEdge* અચલ untknp = srcp()->untknEdgep();
    જો (આ == untknp) {
        label += " / F";
    } નહીંતર જો (untknp) {
        label += " / T";
    }
    પાછા label;
}

//######################################################################
// CfgGraph method definitions

સ્થિર શૂન્ય cfgOrderVisitBlock(std::vector<CfgBlock*>& postOrderEnumeration, CfgBlock* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    જો (CfgBlock* અચલ takenp = bbp->takenp()) {
        જો (!takenp->user()) cfgOrderVisitBlock(postOrderEnumeration, takenp);
        જો (CfgBlock* અચલ untknp = bbp->untknp()) {
            જો (!untknp->user()) cfgOrderVisitBlock(postOrderEnumeration, untknp);
        }
    }
    // Add to post order enumeration
    postOrderEnumeration.emplace_back(bbp);
};

શૂન્ય CfgGraph::rpoBlocks() {
    UASSERT_OBJ(m_nEdits != m_nLastOrdered, m_enterp, "Redundant 'CfgGraph::order' call");
    m_nLastOrdered = m_nEdits;

    // Reset marks
    માટે (V3GraphVertex& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::vector<CfgBlock*> postOrderEnumeration;
    postOrderEnumeration.reserve(m_nBlocks);
    cfgOrderVisitBlock(postOrderEnumeration, m_enterp);
    UASSERT_OBJ(postOrderEnumeration.size() == m_nBlocks, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    માટે (size_t i = 0; i < postOrderEnumeration.size(); ++i) {
        CfgBlock* અચલ bbp = postOrderEnumeration[m_nBlocks - 1 - i];
        bbp->m_rpo_સંખ્યા = i;
        vertices().unlink(bbp);
        vertices().linkBack(bbp);
    }

    // Assign edge IDs
    size_t edge_ગણતરી = 0;
    માટે (V3GraphVertex& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        માટે (V3GraphEdge& e : v.outEdges()) static_cast<CfgEdge&>(e).m_id = edge_ગણતરી++;
    }
    UASSERT_OBJ(edge_ગણતરી == m_nEdges, m_enterp, "Inconsistent edge count");
}

તાર્કિક CfgGraph::containsLoop() અચલ {
    માટે (અચલ V3GraphVertex& vtx : vertices()) {
        અચલ CfgBlock& current = static_cast<અચલ CfgBlock&>(vtx);
        માટે (અચલ V3GraphEdge& edge : current.outEdges()) {
            અચલ CfgBlock& successor = *static_cast<અચલ CfgBlock*>(edge.ટોચ());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            જો (successor.id() < current.id()) પાછા સાચું;
        }
    }
    પાછા ખોટું;
}

શૂન્ય CfgGraph::minimize() {
    // Remove empty blocks (except enter and exit)
    માટે (V3GraphVertex* અચલ vtxp : vertices().unlinkable()) {
        CfgBlock* અચલ bbp = static_cast<CfgBlock*>(vtxp);
        જો (bbp->isEnter()) ચાલુ;
        જો (bbp->isExit()) ચાલુ;
        જો (!bbp->stmtps().empty()) ચાલુ;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        CfgBlock* અચલ succp = bbp->takenp();
        માટે (V3GraphEdge* અચલ edgep : bbp->inEdges().unlinkable()) edgep->relink_ટોચ(succp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(bbp->unlinkDelete(આ), bbp);
    }

    // Combine sequential blocks
    માટે (V3GraphVertex* અચલ vtxp : vertices().unlinkable()) {
        CfgBlock* અચલ srcp = static_cast<CfgBlock*>(vtxp);
        જો (srcp->isExit()) ચાલુ;
        જો (srcp->isBranch()) ચાલુ;
        CfgBlock* અચલ dstp = srcp->takenp();
        જો (dstp->isJoin()) ચાલુ;
        // Combine them
        જો (srcp->isEnter()) m_enterp = dstp;
        std::vector<Ast_ગાંઠ_Stmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.size() + dstp->m_stmtps.size());
        stmtps.ઉમેરો(stmtps.end(), dstp->m_stmtps.begin(), dstp->m_stmtps.end());
        dstp->m_stmtps = std::move(stmtps);
        માટે (V3GraphEdge* અચલ edgep : srcp->inEdges().unlinkable()) edgep->relink_ટોચ(dstp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(srcp->unlinkDelete(આ), srcp);
    }

    જો (m_nEdits != m_nLastOrdered) rpoBlocks();
    જો (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

શૂન્ય CfgGraph::breakCriticalEdges() {
    // Gather critical edges
    std::vector<CfgEdge*> criticalEdges;
    criticalEdges.reserve(m_nEdges);
    માટે (V3GraphVertex& vtx : vertices()) {
        અચલ CfgBlock& bb = static_cast<અચલ CfgBlock&>(vtx);
        જો (!bb.isBranch()) ચાલુ;
        માટે (V3GraphEdge& edge : vtx.outEdges()) {
            અચલ CfgBlock& succ = static_cast<અચલ CfgBlock&>(*edge.ટોચ());
            જો (!succ.isJoin()) ચાલુ;
            criticalEdges.emplace_back(static_cast<CfgEdge*>(&edge));
        }
    }
    // Insert blocks
    માટે (CfgEdge* અચલ edgep : criticalEdges) {
        CfgBlock* અચલ newp = addBlock();
        addTakenEdge(newp, edgep->dstp());
        edgep->relink_ટોચ(newp);
    }

    જો (m_nEdits != m_nLastOrdered) rpoBlocks();
    જો (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-breakCriticalEdges");
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
    અચલ auto chaseEdge = [&](CfgEdge* edgep) -> CfgEdge* {
        જ્યારે (સાચું) {
            CfgBlock* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            જો (dstp->isJoin()) પાછા edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            જ્યારે (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                જો (!dstp) પાછા nullptr;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->takenEdgep();
        }
    };

    // Walk down both paths
    CfgEdge* અચલ takenEdgep = chaseEdge(bbp->takenEdgep());
    જો (!takenEdgep) પાછા nullptr;
    CfgEdge* અચલ untknEdgep = chaseEdge(bbp->untknEdgep());
    જો (!untknEdgep) પાછા nullptr;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    જો (takenEdgep->dstp() != untknEdgep->dstp()) પાછા nullptr;

    // Pick up the common successor
    CfgBlock* અચલ succp = takenEdgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    જો (succp->isTwoWayJoin()) પાછા succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    CfgBlock* અચલ joinp = addBlock();
    addTakenEdge(joinp, succp);
    takenEdgep->relink_ટોચ(joinp);
    untknEdgep->relink_ટોચ(joinp);
    પાછા joinp;
}

તાર્કિક CfgGraph::ઉમેરો_Two_Way_Joins() {
    // Reset marks
    માટે (V3GraphVertex& v : vertices()) v.user(0);

    તાર્કિક isSeriesParallel = સાચું;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    માટે (V3GraphVertex& v : vertices()) {
        CfgBlock& bb = static_cast<CfgBlock&>(v);
        // Skip if already visited
        જો (bb.user()) ચાલુ;
        // Skip if not a branch
        જો (!bb.isBranch()) ચાલુ;
        // Fix it up, record if failed
        જો (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = ખોટું;
    }

    જો (m_nEdits != m_nLastOrdered) rpoBlocks();
    જો (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    પાછા isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

અચલ CfgBlock* Cfg_Dominator_વૃક્ષ::intersect(અચલ CfgBlock* ap, અચલ CfgBlock* bp) {
    જ્યારે (ap != bp) {
        જ્યારે (*ap > *bp) ap = m_bb2Idom[*ap];
        જ્યારે (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    પાછા ap;
}

Cfg_Dominator_વૃક્ષ::Cfg_Dominator_વૃક્ષ(અચલ CfgGraph& cfg)
    : m_bb2Idom{cfg.makeBlockMap<અચલ CfgBlock*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    માટે (તાર્કિક changed = સાચું; changed;) {
        changed = ખોટું;
        // For each vertex except enter block
        માટે (અચલ V3GraphVertex& vtx : cfg.vertices()) {
            અચલ CfgBlock& curr = static_cast<અચલ CfgBlock&>(vtx);
            જો (curr.isEnter()) ચાલુ;  // Skip entry block

            // For each predecessor of current block
            અચલ CfgBlock* idom = nullptr;
            માટે (અચલ V3GraphEdge& edge : curr.inEdges()) {
                અચલ CfgBlock& pred = static_cast<અચલ CfgBlock&>(*edge.fromp());
                // Skip if perdecessor not yet processed
                જો (!m_bb2Idom[pred]) ચાલુ;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            જો (idom == m_bb2Idom[curr]) ચાલુ;
            m_bb2Idom[curr] = idom;
            changed = સાચું;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = nullptr;
}
