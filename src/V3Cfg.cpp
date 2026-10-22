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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Cfg.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3EmitV.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// CfgBlock method definitions

std::string CfgBlock::name() സ്ഥിരം {
    std::stringstream ss;
    ss << "BB " + std::to_string(id()) + ":\n";
    വേണ്ടി (Ast_കണ്ണി* nodep : m_stmtps) {
        എങ്കിൽ (സ്ഥിരം AstIf* സ്ഥിരം ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debug_Verilog_For_വൃക്ഷം(ifp->condp(), ss);
            ss << ") ...";
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstLoopTest* സ്ഥിരം testp = VN_CAST(nodep, LoopTest)) {
            ss << "if  (!";
            V3EmitV::debug_Verilog_For_വൃക്ഷം(testp->condp(), ss);
            ss << ") break;";
        } അല്ലെങ്കിൽ {
            V3EmitV::debug_Verilog_For_വൃക്ഷം(nodep, ss);
        }
    }
    std::string text = VString::replaceSubstr(
        VString::replaceSubstr(ss.str(), "\n", "\\l        "), "\"", "\\\"");
    എങ്കിൽ (isEnter()) text = "**ENTER**\n" + text;
    എങ്കിൽ (isExit()) text = text + "\n**EXIT**";
    തിരികെ text;
}
std::string CfgBlock::dotShape() സ്ഥിരം { തിരികെ "rect"; }
std::string CfgBlock::dotRank() സ്ഥിരം {
    എങ്കിൽ (isEnter()) തിരികെ "source";
    എങ്കിൽ (isExit()) തിരികെ "sink";
    തിരികെ "";
}

//######################################################################
// CfgEdge method definitions

std::string CfgEdge::dotLabel() സ്ഥിരം {
    std::string label = "E" + std::to_string(id());
    സ്ഥിരം CfgEdge* സ്ഥിരം untknp = srcp()->untknEdgep();
    എങ്കിൽ (ഇത് == untknp) {
        label += " / F";
    } അല്ലെങ്കിൽ എങ്കിൽ (untknp) {
        label += " / T";
    }
    തിരികെ label;
}

//######################################################################
// CfgGraph method definitions

സ്ഥിതം ശൂന്യം cfgOrderVisitBlock(std::vector<CfgBlock*>& postOrderEnumeration, CfgBlock* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    എങ്കിൽ (CfgBlock* സ്ഥിരം takenp = bbp->takenp()) {
        എങ്കിൽ (!takenp->user()) cfgOrderVisitBlock(postOrderEnumeration, takenp);
        എങ്കിൽ (CfgBlock* സ്ഥിരം untknp = bbp->untknp()) {
            എങ്കിൽ (!untknp->user()) cfgOrderVisitBlock(postOrderEnumeration, untknp);
        }
    }
    // Add to post order enumeration
    postOrderEnumeration.emplace_back(bbp);
};

ശൂന്യം CfgGraph::rpoBlocks() {
    UASSERT_OBJ(m_nEdits != m_nLastOrdered, m_enterp, "Redundant 'CfgGraph::order' call");
    m_nLastOrdered = m_nEdits;

    // Reset marks
    വേണ്ടി (V3GraphVertex& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::vector<CfgBlock*> postOrderEnumeration;
    postOrderEnumeration.reserve(m_nBlocks);
    cfgOrderVisitBlock(postOrderEnumeration, m_enterp);
    UASSERT_OBJ(postOrderEnumeration.size() == m_nBlocks, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    വേണ്ടി (size_t i = 0; i < postOrderEnumeration.size(); ++i) {
        CfgBlock* സ്ഥിരം bbp = postOrderEnumeration[m_nBlocks - 1 - i];
        bbp->m_rpo_സംഖ്യ = i;
        vertices().unlink(bbp);
        vertices().linkBack(bbp);
    }

    // Assign edge IDs
    size_t edge_എണ്ണം = 0;
    വേണ്ടി (V3GraphVertex& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        വേണ്ടി (V3GraphEdge& e : v.outEdges()) static_cast<CfgEdge&>(e).m_id = edge_എണ്ണം++;
    }
    UASSERT_OBJ(edge_എണ്ണം == m_nEdges, m_enterp, "Inconsistent edge count");
}

തർക്കം CfgGraph::containsLoop() സ്ഥിരം {
    വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : vertices()) {
        സ്ഥിരം CfgBlock& current = static_cast<സ്ഥിരം CfgBlock&>(vtx);
        വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : current.outEdges()) {
            സ്ഥിരം CfgBlock& successor = *static_cast<സ്ഥിരം CfgBlock*>(edge.മുകൾ());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            എങ്കിൽ (successor.id() < current.id()) തിരികെ സത്യം;
        }
    }
    തിരികെ അസത്യം;
}

ശൂന്യം CfgGraph::minimize() {
    // Remove empty blocks (except enter and exit)
    വേണ്ടി (V3GraphVertex* സ്ഥിരം vtxp : vertices().unlinkable()) {
        CfgBlock* സ്ഥിരം bbp = static_cast<CfgBlock*>(vtxp);
        എങ്കിൽ (bbp->isEnter()) തുടരുക;
        എങ്കിൽ (bbp->isExit()) തുടരുക;
        എങ്കിൽ (!bbp->stmtps().empty()) തുടരുക;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        CfgBlock* സ്ഥിരം succp = bbp->takenp();
        വേണ്ടി (V3GraphEdge* സ്ഥിരം edgep : bbp->inEdges().unlinkable()) edgep->relink_മുകൾ(succp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(bbp->unlinkDelete(ഇത്), bbp);
    }

    // Combine sequential blocks
    വേണ്ടി (V3GraphVertex* സ്ഥിരം vtxp : vertices().unlinkable()) {
        CfgBlock* സ്ഥിരം srcp = static_cast<CfgBlock*>(vtxp);
        എങ്കിൽ (srcp->isExit()) തുടരുക;
        എങ്കിൽ (srcp->isBranch()) തുടരുക;
        CfgBlock* സ്ഥിരം dstp = srcp->takenp();
        എങ്കിൽ (dstp->isJoin()) തുടരുക;
        // Combine them
        എങ്കിൽ (srcp->isEnter()) m_enterp = dstp;
        std::vector<Ast_കണ്ണി_Stmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.size() + dstp->m_stmtps.size());
        stmtps.തിരുകുക(stmtps.end(), dstp->m_stmtps.begin(), dstp->m_stmtps.end());
        dstp->m_stmtps = std::move(stmtps);
        വേണ്ടി (V3GraphEdge* സ്ഥിരം edgep : srcp->inEdges().unlinkable()) edgep->relink_മുകൾ(dstp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(srcp->unlinkDelete(ഇത്), srcp);
    }

    എങ്കിൽ (m_nEdits != m_nLastOrdered) rpoBlocks();
    എങ്കിൽ (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

ശൂന്യം CfgGraph::breakCriticalEdges() {
    // Gather critical edges
    std::vector<CfgEdge*> criticalEdges;
    criticalEdges.reserve(m_nEdges);
    വേണ്ടി (V3GraphVertex& vtx : vertices()) {
        സ്ഥിരം CfgBlock& bb = static_cast<സ്ഥിരം CfgBlock&>(vtx);
        എങ്കിൽ (!bb.isBranch()) തുടരുക;
        വേണ്ടി (V3GraphEdge& edge : vtx.outEdges()) {
            സ്ഥിരം CfgBlock& succ = static_cast<സ്ഥിരം CfgBlock&>(*edge.മുകൾ());
            എങ്കിൽ (!succ.isJoin()) തുടരുക;
            criticalEdges.emplace_back(static_cast<CfgEdge*>(&edge));
        }
    }
    // Insert blocks
    വേണ്ടി (CfgEdge* സ്ഥിരം edgep : criticalEdges) {
        CfgBlock* സ്ഥിരം newp = addBlock();
        addTakenEdge(newp, edgep->dstp());
        edgep->relink_മുകൾ(newp);
    }

    എങ്കിൽ (m_nEdits != m_nLastOrdered) rpoBlocks();
    എങ്കിൽ (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-breakCriticalEdges");
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
    സ്ഥിരം auto chaseEdge = [&](CfgEdge* edgep) -> CfgEdge* {
        ആകുമ്പോൾ (സത്യം) {
            CfgBlock* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            എങ്കിൽ (dstp->isJoin()) തിരികെ edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            ആകുമ്പോൾ (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                എങ്കിൽ (!dstp) തിരികെ nullptr;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->takenEdgep();
        }
    };

    // Walk down both paths
    CfgEdge* സ്ഥിരം takenEdgep = chaseEdge(bbp->takenEdgep());
    എങ്കിൽ (!takenEdgep) തിരികെ nullptr;
    CfgEdge* സ്ഥിരം untknEdgep = chaseEdge(bbp->untknEdgep());
    എങ്കിൽ (!untknEdgep) തിരികെ nullptr;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    എങ്കിൽ (takenEdgep->dstp() != untknEdgep->dstp()) തിരികെ nullptr;

    // Pick up the common successor
    CfgBlock* സ്ഥിരം succp = takenEdgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    എങ്കിൽ (succp->isTwoWayJoin()) തിരികെ succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    CfgBlock* സ്ഥിരം joinp = addBlock();
    addTakenEdge(joinp, succp);
    takenEdgep->relink_മുകൾ(joinp);
    untknEdgep->relink_മുകൾ(joinp);
    തിരികെ joinp;
}

തർക്കം CfgGraph::തിരുകുക_Two_Way_Joins() {
    // Reset marks
    വേണ്ടി (V3GraphVertex& v : vertices()) v.user(0);

    തർക്കം isSeriesParallel = സത്യം;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    വേണ്ടി (V3GraphVertex& v : vertices()) {
        CfgBlock& bb = static_cast<CfgBlock&>(v);
        // Skip if already visited
        എങ്കിൽ (bb.user()) തുടരുക;
        // Skip if not a branch
        എങ്കിൽ (!bb.isBranch()) തുടരുക;
        // Fix it up, record if failed
        എങ്കിൽ (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = അസത്യം;
    }

    എങ്കിൽ (m_nEdits != m_nLastOrdered) rpoBlocks();
    എങ്കിൽ (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    തിരികെ isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

സ്ഥിരം CfgBlock* Cfg_Dominator_വൃക്ഷം::intersect(സ്ഥിരം CfgBlock* ap, സ്ഥിരം CfgBlock* bp) {
    ആകുമ്പോൾ (ap != bp) {
        ആകുമ്പോൾ (*ap > *bp) ap = m_bb2Idom[*ap];
        ആകുമ്പോൾ (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    തിരികെ ap;
}

Cfg_Dominator_വൃക്ഷം::Cfg_Dominator_വൃക്ഷം(സ്ഥിരം CfgGraph& cfg)
    : m_bb2Idom{cfg.makeBlockMap<സ്ഥിരം CfgBlock*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    വേണ്ടി (തർക്കം changed = സത്യം; changed;) {
        changed = അസത്യം;
        // For each vertex except enter block
        വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : cfg.vertices()) {
            സ്ഥിരം CfgBlock& curr = static_cast<സ്ഥിരം CfgBlock&>(vtx);
            എങ്കിൽ (curr.isEnter()) തുടരുക;  // Skip entry block

            // For each predecessor of current block
            സ്ഥിരം CfgBlock* idom = nullptr;
            വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : curr.inEdges()) {
                സ്ഥിരം CfgBlock& pred = static_cast<സ്ഥിരം CfgBlock&>(*edge.fromp());
                // Skip if perdecessor not yet processed
                എങ്കിൽ (!m_bb2Idom[pred]) തുടരുക;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            എങ്കിൽ (idom == m_bb2Idom[curr]) തുടരുക;
            m_bb2Idom[curr] = idom;
            changed = സത്യം;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = nullptr;
}
