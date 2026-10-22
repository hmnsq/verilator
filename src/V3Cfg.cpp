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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Cfg.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3EmitV.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// CfgBlock method definitions

std::string CfgBlock::name() स्थिरांक {
    std::stringstream ss;
    ss << "BB " + std::to_string(id()) + ":\n";
    साठी (Ast_गाठ* nodep : m_stmtps) {
        जर (स्थिरांक AstIf* स्थिरांक ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debug_Verilog_For_वृक्ष(ifp->condp(), ss);
            ss << ") ...";
        } अन्यथा जर (स्थिरांक AstLoopTest* स्थिरांक testp = VN_CAST(nodep, LoopTest)) {
            ss << "if  (!";
            V3EmitV::debug_Verilog_For_वृक्ष(testp->condp(), ss);
            ss << ") break;";
        } अन्यथा {
            V3EmitV::debug_Verilog_For_वृक्ष(nodep, ss);
        }
    }
    std::string text = VString::replaceSubstr(
        VString::replaceSubstr(ss.str(), "\n", "\\l        "), "\"", "\\\"");
    जर (isEnter()) text = "**ENTER**\n" + text;
    जर (isExit()) text = text + "\n**EXIT**";
    परत text;
}
std::string CfgBlock::dotShape() स्थिरांक { परत "rect"; }
std::string CfgBlock::dotRank() स्थिरांक {
    जर (isEnter()) परत "source";
    जर (isExit()) परत "sink";
    परत "";
}

//######################################################################
// CfgEdge method definitions

std::string CfgEdge::dotLabel() स्थिरांक {
    std::string label = "E" + std::to_string(id());
    स्थिरांक CfgEdge* स्थिरांक untknp = srcp()->untknEdgep();
    जर (हे == untknp) {
        label += " / F";
    } अन्यथा जर (untknp) {
        label += " / T";
    }
    परत label;
}

//######################################################################
// CfgGraph method definitions

स्थिर शून्य cfgOrderVisitBlock(std::vector<CfgBlock*>& postOrderEnumeration, CfgBlock* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    जर (CfgBlock* स्थिरांक takenp = bbp->takenp()) {
        जर (!takenp->user()) cfgOrderVisitBlock(postOrderEnumeration, takenp);
        जर (CfgBlock* स्थिरांक untknp = bbp->untknp()) {
            जर (!untknp->user()) cfgOrderVisitBlock(postOrderEnumeration, untknp);
        }
    }
    // Add to post order enumeration
    postOrderEnumeration.emplace_back(bbp);
};

शून्य CfgGraph::rpoBlocks() {
    UASSERT_OBJ(m_nEdits != m_nLastOrdered, m_enterp, "Redundant 'CfgGraph::order' call");
    m_nLastOrdered = m_nEdits;

    // Reset marks
    साठी (V3GraphVertex& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::vector<CfgBlock*> postOrderEnumeration;
    postOrderEnumeration.reserve(m_nBlocks);
    cfgOrderVisitBlock(postOrderEnumeration, m_enterp);
    UASSERT_OBJ(postOrderEnumeration.size() == m_nBlocks, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    साठी (माप_प्रकार i = 0; i < postOrderEnumeration.size(); ++i) {
        CfgBlock* स्थिरांक bbp = postOrderEnumeration[m_nBlocks - 1 - i];
        bbp->m_rpo_संख्या = i;
        vertices().unlink(bbp);
        vertices().linkBack(bbp);
    }

    // Assign edge IDs
    माप_प्रकार edge_मोजणी = 0;
    साठी (V3GraphVertex& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        साठी (V3GraphEdge& e : v.outEdges()) static_cast<CfgEdge&>(e).m_id = edge_मोजणी++;
    }
    UASSERT_OBJ(edge_मोजणी == m_nEdges, m_enterp, "Inconsistent edge count");
}

तार्किक CfgGraph::containsLoop() स्थिरांक {
    साठी (स्थिरांक V3GraphVertex& vtx : vertices()) {
        स्थिरांक CfgBlock& current = static_cast<स्थिरांक CfgBlock&>(vtx);
        साठी (स्थिरांक V3GraphEdge& edge : current.outEdges()) {
            स्थिरांक CfgBlock& successor = *static_cast<स्थिरांक CfgBlock*>(edge.शीर्ष());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            जर (successor.id() < current.id()) परत सत्य;
        }
    }
    परत असत्य;
}

शून्य CfgGraph::minimize() {
    // Remove empty blocks (except enter and exit)
    साठी (V3GraphVertex* स्थिरांक vtxp : vertices().unlinkable()) {
        CfgBlock* स्थिरांक bbp = static_cast<CfgBlock*>(vtxp);
        जर (bbp->isEnter()) सुरूठेवा;
        जर (bbp->isExit()) सुरूठेवा;
        जर (!bbp->stmtps().empty()) सुरूठेवा;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        CfgBlock* स्थिरांक succp = bbp->takenp();
        साठी (V3GraphEdge* स्थिरांक edgep : bbp->inEdges().unlinkable()) edgep->relink_शीर्ष(succp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(bbp->unlinkDelete(हे), bbp);
    }

    // Combine sequential blocks
    साठी (V3GraphVertex* स्थिरांक vtxp : vertices().unlinkable()) {
        CfgBlock* स्थिरांक srcp = static_cast<CfgBlock*>(vtxp);
        जर (srcp->isExit()) सुरूठेवा;
        जर (srcp->isBranch()) सुरूठेवा;
        CfgBlock* स्थिरांक dstp = srcp->takenp();
        जर (dstp->isJoin()) सुरूठेवा;
        // Combine them
        जर (srcp->isEnter()) m_enterp = dstp;
        std::vector<Ast_गाठ_Stmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.size() + dstp->m_stmtps.size());
        stmtps.घाला(stmtps.end(), dstp->m_stmtps.begin(), dstp->m_stmtps.end());
        dstp->m_stmtps = std::move(stmtps);
        साठी (V3GraphEdge* स्थिरांक edgep : srcp->inEdges().unlinkable()) edgep->relink_शीर्ष(dstp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(srcp->unlinkDelete(हे), srcp);
    }

    जर (m_nEdits != m_nLastOrdered) rpoBlocks();
    जर (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

शून्य CfgGraph::breakCriticalEdges() {
    // Gather critical edges
    std::vector<CfgEdge*> criticalEdges;
    criticalEdges.reserve(m_nEdges);
    साठी (V3GraphVertex& vtx : vertices()) {
        स्थिरांक CfgBlock& bb = static_cast<स्थिरांक CfgBlock&>(vtx);
        जर (!bb.isBranch()) सुरूठेवा;
        साठी (V3GraphEdge& edge : vtx.outEdges()) {
            स्थिरांक CfgBlock& succ = static_cast<स्थिरांक CfgBlock&>(*edge.शीर्ष());
            जर (!succ.isJoin()) सुरूठेवा;
            criticalEdges.emplace_back(static_cast<CfgEdge*>(&edge));
        }
    }
    // Insert blocks
    साठी (CfgEdge* स्थिरांक edgep : criticalEdges) {
        CfgBlock* स्थिरांक newp = addBlock();
        addTakenEdge(newp, edgep->dstp());
        edgep->relink_शीर्ष(newp);
    }

    जर (m_nEdits != m_nLastOrdered) rpoBlocks();
    जर (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-breakCriticalEdges");
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
    स्थिरांक स्वतः chaseEdge = [&](CfgEdge* edgep) -> CfgEdge* {
        पर्यंत (सत्य) {
            CfgBlock* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            जर (dstp->isJoin()) परत edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            पर्यंत (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                जर (!dstp) परत nullptr;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->takenEdgep();
        }
    };

    // Walk down both paths
    CfgEdge* स्थिरांक takenEdgep = chaseEdge(bbp->takenEdgep());
    जर (!takenEdgep) परत nullptr;
    CfgEdge* स्थिरांक untknEdgep = chaseEdge(bbp->untknEdgep());
    जर (!untknEdgep) परत nullptr;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    जर (takenEdgep->dstp() != untknEdgep->dstp()) परत nullptr;

    // Pick up the common successor
    CfgBlock* स्थिरांक succp = takenEdgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    जर (succp->isTwoWayJoin()) परत succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    CfgBlock* स्थिरांक joinp = addBlock();
    addTakenEdge(joinp, succp);
    takenEdgep->relink_शीर्ष(joinp);
    untknEdgep->relink_शीर्ष(joinp);
    परत joinp;
}

तार्किक CfgGraph::घाला_Two_Way_Joins() {
    // Reset marks
    साठी (V3GraphVertex& v : vertices()) v.user(0);

    तार्किक isSeriesParallel = सत्य;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    साठी (V3GraphVertex& v : vertices()) {
        CfgBlock& bb = static_cast<CfgBlock&>(v);
        // Skip if already visited
        जर (bb.user()) सुरूठेवा;
        // Skip if not a branch
        जर (!bb.isBranch()) सुरूठेवा;
        // Fix it up, record if failed
        जर (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = असत्य;
    }

    जर (m_nEdits != m_nLastOrdered) rpoBlocks();
    जर (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    परत isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

स्थिरांक CfgBlock* Cfg_Dominator_वृक्ष::intersect(स्थिरांक CfgBlock* ap, स्थिरांक CfgBlock* bp) {
    पर्यंत (ap != bp) {
        पर्यंत (*ap > *bp) ap = m_bb2Idom[*ap];
        पर्यंत (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    परत ap;
}

Cfg_Dominator_वृक्ष::Cfg_Dominator_वृक्ष(स्थिरांक CfgGraph& cfg)
    : m_bb2Idom{cfg.makeBlockMap<स्थिरांक CfgBlock*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    साठी (तार्किक changed = सत्य; changed;) {
        changed = असत्य;
        // For each vertex except enter block
        साठी (स्थिरांक V3GraphVertex& vtx : cfg.vertices()) {
            स्थिरांक CfgBlock& curr = static_cast<स्थिरांक CfgBlock&>(vtx);
            जर (curr.isEnter()) सुरूठेवा;  // Skip entry block

            // For each predecessor of current block
            स्थिरांक CfgBlock* idom = nullptr;
            साठी (स्थिरांक V3GraphEdge& edge : curr.inEdges()) {
                स्थिरांक CfgBlock& pred = static_cast<स्थिरांक CfgBlock&>(*edge.fromp());
                // Skip if perdecessor not yet processed
                जर (!m_bb2Idom[pred]) सुरूठेवा;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            जर (idom == m_bb2Idom[curr]) सुरूठेवा;
            m_bb2Idom[curr] = idom;
            changed = सत्य;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = nullptr;
}
