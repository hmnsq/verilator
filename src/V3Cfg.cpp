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

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Cfg.h"

#समावेश "V3Ast.h"
#समावेश "V3EmitV.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// CfgBlock method definitions

std::string CfgBlock::name() स्थिर {
    std::stringstream ss;
    ss << "BB " + std::to_string(id()) + ":\n";
    क्रम (Ast_गाँठ* nodep : m_stmtps) {
        अगर (स्थिर AstIf* स्थिर ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debug_Verilog_For_वृक्ष(ifp->condp(), ss);
            ss << ") ...";
        } अन्यथा अगर (स्थिर AstLoopTest* स्थिर testp = VN_CAST(nodep, LoopTest)) {
            ss << "if  (!";
            V3EmitV::debug_Verilog_For_वृक्ष(testp->condp(), ss);
            ss << ") break;";
        } अन्यथा {
            V3EmitV::debug_Verilog_For_वृक्ष(nodep, ss);
        }
    }
    std::string text = VString::replaceSubstr(
        VString::replaceSubstr(ss.str(), "\n", "\\l        "), "\"", "\\\"");
    अगर (isEnter()) text = "**ENTER**\n" + text;
    अगर (isExit()) text = text + "\n**EXIT**";
    वापस text;
}
std::string CfgBlock::dotShape() स्थिर { वापस "rect"; }
std::string CfgBlock::dotRank() स्थिर {
    अगर (isEnter()) वापस "source";
    अगर (isExit()) वापस "sink";
    वापस "";
}

//######################################################################
// CfgEdge method definitions

std::string CfgEdge::dotLabel() स्थिर {
    std::string label = "E" + std::to_string(id());
    स्थिर CfgEdge* स्थिर untknp = srcp()->untknEdgep();
    अगर (यह == untknp) {
        label += " / F";
    } अन्यथा अगर (untknp) {
        label += " / T";
    }
    वापस label;
}

//######################################################################
// CfgGraph method definitions

अटल व्योम cfgOrderVisitBlock(std::vector<CfgBlock*>& postOrderEnumeration, CfgBlock* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    अगर (CfgBlock* स्थिर takenp = bbp->takenp()) {
        अगर (!takenp->user()) cfgOrderVisitBlock(postOrderEnumeration, takenp);
        अगर (CfgBlock* स्थिर untknp = bbp->untknp()) {
            अगर (!untknp->user()) cfgOrderVisitBlock(postOrderEnumeration, untknp);
        }
    }
    // Add to post order enumeration
    postOrderEnumeration.emplace_back(bbp);
};

व्योम CfgGraph::rpoBlocks() {
    UASSERT_OBJ(m_nEdits != m_nLastOrdered, m_enterp, "Redundant 'CfgGraph::order' call");
    m_nLastOrdered = m_nEdits;

    // Reset marks
    क्रम (V3GraphVertex& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::vector<CfgBlock*> postOrderEnumeration;
    postOrderEnumeration.reserve(m_nBlocks);
    cfgOrderVisitBlock(postOrderEnumeration, m_enterp);
    UASSERT_OBJ(postOrderEnumeration.आकार() == m_nBlocks, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    क्रम (माप_प्रकार i = 0; i < postOrderEnumeration.आकार(); ++i) {
        CfgBlock* स्थिर bbp = postOrderEnumeration[m_nBlocks - 1 - i];
        bbp->m_rpo_संख्या = i;
        vertices().unlink(bbp);
        vertices().linkBack(bbp);
    }

    // Assign edge IDs
    माप_प्रकार edge_गिनती = 0;
    क्रम (V3GraphVertex& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        क्रम (V3GraphEdge& e : v.outEdges()) static_cast<CfgEdge&>(e).m_id = edge_गिनती++;
    }
    UASSERT_OBJ(edge_गिनती == m_nEdges, m_enterp, "Inconsistent edge count");
}

bool CfgGraph::containsLoop() स्थिर {
    क्रम (स्थिर V3GraphVertex& vtx : vertices()) {
        स्थिर CfgBlock& current = static_cast<स्थिर CfgBlock&>(vtx);
        क्रम (स्थिर V3GraphEdge& edge : current.outEdges()) {
            स्थिर CfgBlock& successor = *static_cast<स्थिर CfgBlock*>(edge.शीर्ष());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            अगर (successor.id() < current.id()) वापस true;
        }
    }
    वापस false;
}

व्योम CfgGraph::minimize() {
    // Remove empty blocks (except enter and exit)
    क्रम (V3GraphVertex* स्थिर vtxp : vertices().unlinkable()) {
        CfgBlock* स्थिर bbp = static_cast<CfgBlock*>(vtxp);
        अगर (bbp->isEnter()) जारी;
        अगर (bbp->isExit()) जारी;
        अगर (!bbp->stmtps().empty()) जारी;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        CfgBlock* स्थिर succp = bbp->takenp();
        क्रम (V3GraphEdge* स्थिर edgep : bbp->inEdges().unlinkable()) edgep->relink_शीर्ष(succp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(bbp->unlinkDelete(यह), bbp);
    }

    // Combine sequential blocks
    क्रम (V3GraphVertex* स्थिर vtxp : vertices().unlinkable()) {
        CfgBlock* स्थिर srcp = static_cast<CfgBlock*>(vtxp);
        अगर (srcp->isExit()) जारी;
        अगर (srcp->isBranch()) जारी;
        CfgBlock* स्थिर dstp = srcp->takenp();
        अगर (dstp->isJoin()) जारी;
        // Combine them
        अगर (srcp->isEnter()) m_enterp = dstp;
        std::vector<Ast_गाँठ_Stmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.आकार() + dstp->m_stmtps.आकार());
        stmtps.जोड़ो(stmtps.end(), dstp->m_stmtps.begin(), dstp->m_stmtps.end());
        dstp->m_stmtps = std::move(stmtps);
        क्रम (V3GraphEdge* स्थिर edgep : srcp->inEdges().unlinkable()) edgep->relink_शीर्ष(dstp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(srcp->unlinkDelete(यह), srcp);
    }

    अगर (m_nEdits != m_nLastOrdered) rpoBlocks();
    अगर (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

व्योम CfgGraph::breakCriticalEdges() {
    // Gather critical edges
    std::vector<CfgEdge*> criticalEdges;
    criticalEdges.reserve(m_nEdges);
    क्रम (V3GraphVertex& vtx : vertices()) {
        स्थिर CfgBlock& bb = static_cast<स्थिर CfgBlock&>(vtx);
        अगर (!bb.isBranch()) जारी;
        क्रम (V3GraphEdge& edge : vtx.outEdges()) {
            स्थिर CfgBlock& succ = static_cast<स्थिर CfgBlock&>(*edge.शीर्ष());
            अगर (!succ.isJoin()) जारी;
            criticalEdges.emplace_back(static_cast<CfgEdge*>(&edge));
        }
    }
    // Insert blocks
    क्रम (CfgEdge* स्थिर edgep : criticalEdges) {
        CfgBlock* स्थिर newp = addBlock();
        addTakenEdge(newp, edgep->dstp());
        edgep->relink_शीर्ष(newp);
    }

    अगर (m_nEdits != m_nLastOrdered) rpoBlocks();
    अगर (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-breakCriticalEdges");
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
    स्थिर स्वतः chaseEdge = [&](CfgEdge* edgep) -> CfgEdge* {
        जबतक (true) {
            CfgBlock* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            अगर (dstp->isJoin()) वापस edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            जबतक (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                अगर (!dstp) वापस nullptr;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->takenEdgep();
        }
    };

    // Walk down both paths
    CfgEdge* स्थिर takenEdgep = chaseEdge(bbp->takenEdgep());
    अगर (!takenEdgep) वापस nullptr;
    CfgEdge* स्थिर untknEdgep = chaseEdge(bbp->untknEdgep());
    अगर (!untknEdgep) वापस nullptr;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    अगर (takenEdgep->dstp() != untknEdgep->dstp()) वापस nullptr;

    // Pick up the common successor
    CfgBlock* स्थिर succp = takenEdgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    अगर (succp->isTwoWayJoin()) वापस succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    CfgBlock* स्थिर joinp = addBlock();
    addTakenEdge(joinp, succp);
    takenEdgep->relink_शीर्ष(joinp);
    untknEdgep->relink_शीर्ष(joinp);
    वापस joinp;
}

bool CfgGraph::जोड़ो_Two_Way_Joins() {
    // Reset marks
    क्रम (V3GraphVertex& v : vertices()) v.user(0);

    bool isSeriesParallel = true;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    क्रम (V3GraphVertex& v : vertices()) {
        CfgBlock& bb = static_cast<CfgBlock&>(v);
        // Skip if already visited
        अगर (bb.user()) जारी;
        // Skip if not a branch
        अगर (!bb.isBranch()) जारी;
        // Fix it up, record if failed
        अगर (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = false;
    }

    अगर (m_nEdits != m_nLastOrdered) rpoBlocks();
    अगर (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    वापस isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

स्थिर CfgBlock* Cfg_Dominator_वृक्ष::intersect(स्थिर CfgBlock* ap, स्थिर CfgBlock* bp) {
    जबतक (ap != bp) {
        जबतक (*ap > *bp) ap = m_bb2Idom[*ap];
        जबतक (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    वापस ap;
}

Cfg_Dominator_वृक्ष::Cfg_Dominator_वृक्ष(स्थिर CfgGraph& cfg)
    : m_bb2Idom{cfg.makeBlockMap<स्थिर CfgBlock*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    क्रम (bool changed = true; changed;) {
        changed = false;
        // For each vertex except enter block
        क्रम (स्थिर V3GraphVertex& vtx : cfg.vertices()) {
            स्थिर CfgBlock& curr = static_cast<स्थिर CfgBlock&>(vtx);
            अगर (curr.isEnter()) जारी;  // Skip entry block

            // For each predecessor of current block
            स्थिर CfgBlock* idom = nullptr;
            क्रम (स्थिर V3GraphEdge& edge : curr.inEdges()) {
                स्थिर CfgBlock& pred = static_cast<स्थिर CfgBlock&>(*edge.fromp());
                // Skip if perdecessor not yet processed
                अगर (!m_bb2Idom[pred]) जारी;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            अगर (idom == m_bb2Idom[curr]) जारी;
            m_bb2Idom[curr] = idom;
            changed = true;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = nullptr;
}
