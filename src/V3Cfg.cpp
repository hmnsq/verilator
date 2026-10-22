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

std::string CfgBlock::name() अचर {
    std::stringstream ss;
    ss << "BB " + std::to_string(id()) + ":\n";
    लागि (Ast_गाँठो* nodep : m_stmtps) {
        यदि (अचर AstIf* अचर ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debug_Verilog_For_रूख(ifp->condp(), ss);
            ss << ") ...";
        } अन्यथा यदि (अचर AstLoopTest* अचर testp = VN_CAST(nodep, LoopTest)) {
            ss << "if  (!";
            V3EmitV::debug_Verilog_For_रूख(testp->condp(), ss);
            ss << ") break;";
        } अन्यथा {
            V3EmitV::debug_Verilog_For_रूख(nodep, ss);
        }
    }
    std::string text = VString::replaceSubstr(
        VString::replaceSubstr(ss.str(), "\n", "\\l        "), "\"", "\\\"");
    यदि (isEnter()) text = "**ENTER**\n" + text;
    यदि (isExit()) text = text + "\n**EXIT**";
    फर्काऊ text;
}
std::string CfgBlock::dotShape() अचर { फर्काऊ "rect"; }
std::string CfgBlock::dotRank() अचर {
    यदि (isEnter()) फर्काऊ "source";
    यदि (isExit()) फर्काऊ "sink";
    फर्काऊ "";
}

//######################################################################
// CfgEdge method definitions

std::string CfgEdge::dotLabel() अचर {
    std::string label = "E" + std::to_string(id());
    अचर CfgEdge* अचर untknp = srcp()->untknEdgep();
    यदि (एतत् == untknp) {
        label += " / F";
    } अन्यथा यदि (untknp) {
        label += " / T";
    }
    फर्काऊ label;
}

//######################################################################
// CfgGraph method definitions

स्थिर शून्य cfgOrderVisitBlock(std::vector<CfgBlock*>& postOrderEnumeration, CfgBlock* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    यदि (CfgBlock* अचर takenp = bbp->takenp()) {
        यदि (!takenp->user()) cfgOrderVisitBlock(postOrderEnumeration, takenp);
        यदि (CfgBlock* अचर untknp = bbp->untknp()) {
            यदि (!untknp->user()) cfgOrderVisitBlock(postOrderEnumeration, untknp);
        }
    }
    // Add to post order enumeration
    postOrderEnumeration.emplace_back(bbp);
};

शून्य CfgGraph::rpoBlocks() {
    UASSERT_OBJ(m_nEdits != m_nLastOrdered, m_enterp, "Redundant 'CfgGraph::order' call");
    m_nLastOrdered = m_nEdits;

    // Reset marks
    लागि (V3GraphVertex& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::vector<CfgBlock*> postOrderEnumeration;
    postOrderEnumeration.reserve(m_nBlocks);
    cfgOrderVisitBlock(postOrderEnumeration, m_enterp);
    UASSERT_OBJ(postOrderEnumeration.size() == m_nBlocks, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    लागि (माप_प्रकार i = 0; i < postOrderEnumeration.size(); ++i) {
        CfgBlock* अचर bbp = postOrderEnumeration[m_nBlocks - 1 - i];
        bbp->m_rpo_सङ्ख्या = i;
        vertices().unlink(bbp);
        vertices().linkBack(bbp);
    }

    // Assign edge IDs
    माप_प्रकार edge_गन्ती = 0;
    लागि (V3GraphVertex& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        लागि (V3GraphEdge& e : v.outEdges()) static_cast<CfgEdge&>(e).m_id = edge_गन्ती++;
    }
    UASSERT_OBJ(edge_गन्ती == m_nEdges, m_enterp, "Inconsistent edge count");
}

तार्किक CfgGraph::containsLoop() अचर {
    लागि (अचर V3GraphVertex& vtx : vertices()) {
        अचर CfgBlock& current = static_cast<अचर CfgBlock&>(vtx);
        लागि (अचर V3GraphEdge& edge : current.outEdges()) {
            अचर CfgBlock& successor = *static_cast<अचर CfgBlock*>(edge.शीर्ष());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            यदि (successor.id() < current.id()) फर्काऊ सत्य;
        }
    }
    फर्काऊ असत्य;
}

शून्य CfgGraph::minimize() {
    // Remove empty blocks (except enter and exit)
    लागि (V3GraphVertex* अचर vtxp : vertices().unlinkable()) {
        CfgBlock* अचर bbp = static_cast<CfgBlock*>(vtxp);
        यदि (bbp->isEnter()) जारी;
        यदि (bbp->isExit()) जारी;
        यदि (!bbp->stmtps().empty()) जारी;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        CfgBlock* अचर succp = bbp->takenp();
        लागि (V3GraphEdge* अचर edgep : bbp->inEdges().unlinkable()) edgep->relink_शीर्ष(succp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(bbp->unlinkDelete(एतत्), bbp);
    }

    // Combine sequential blocks
    लागि (V3GraphVertex* अचर vtxp : vertices().unlinkable()) {
        CfgBlock* अचर srcp = static_cast<CfgBlock*>(vtxp);
        यदि (srcp->isExit()) जारी;
        यदि (srcp->isBranch()) जारी;
        CfgBlock* अचर dstp = srcp->takenp();
        यदि (dstp->isJoin()) जारी;
        // Combine them
        यदि (srcp->isEnter()) m_enterp = dstp;
        std::vector<Ast_गाँठो_Stmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.size() + dstp->m_stmtps.size());
        stmtps.घुसाउ(stmtps.end(), dstp->m_stmtps.begin(), dstp->m_stmtps.end());
        dstp->m_stmtps = std::move(stmtps);
        लागि (V3GraphEdge* अचर edgep : srcp->inEdges().unlinkable()) edgep->relink_शीर्ष(dstp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(srcp->unlinkDelete(एतत्), srcp);
    }

    यदि (m_nEdits != m_nLastOrdered) rpoBlocks();
    यदि (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

शून्य CfgGraph::breakCriticalEdges() {
    // Gather critical edges
    std::vector<CfgEdge*> criticalEdges;
    criticalEdges.reserve(m_nEdges);
    लागि (V3GraphVertex& vtx : vertices()) {
        अचर CfgBlock& bb = static_cast<अचर CfgBlock&>(vtx);
        यदि (!bb.isBranch()) जारी;
        लागि (V3GraphEdge& edge : vtx.outEdges()) {
            अचर CfgBlock& succ = static_cast<अचर CfgBlock&>(*edge.शीर्ष());
            यदि (!succ.isJoin()) जारी;
            criticalEdges.emplace_back(static_cast<CfgEdge*>(&edge));
        }
    }
    // Insert blocks
    लागि (CfgEdge* अचर edgep : criticalEdges) {
        CfgBlock* अचर newp = addBlock();
        addTakenEdge(newp, edgep->dstp());
        edgep->relink_शीर्ष(newp);
    }

    यदि (m_nEdits != m_nLastOrdered) rpoBlocks();
    यदि (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-breakCriticalEdges");
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
    अचर स्वतः chaseEdge = [&](CfgEdge* edgep) -> CfgEdge* {
        जबसम्म (सत्य) {
            CfgBlock* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            यदि (dstp->isJoin()) फर्काऊ edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            जबसम्म (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                यदि (!dstp) फर्काऊ nullptr;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->takenEdgep();
        }
    };

    // Walk down both paths
    CfgEdge* अचर takenEdgep = chaseEdge(bbp->takenEdgep());
    यदि (!takenEdgep) फर्काऊ nullptr;
    CfgEdge* अचर untknEdgep = chaseEdge(bbp->untknEdgep());
    यदि (!untknEdgep) फर्काऊ nullptr;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    यदि (takenEdgep->dstp() != untknEdgep->dstp()) फर्काऊ nullptr;

    // Pick up the common successor
    CfgBlock* अचर succp = takenEdgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    यदि (succp->isTwoWayJoin()) फर्काऊ succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    CfgBlock* अचर joinp = addBlock();
    addTakenEdge(joinp, succp);
    takenEdgep->relink_शीर्ष(joinp);
    untknEdgep->relink_शीर्ष(joinp);
    फर्काऊ joinp;
}

तार्किक CfgGraph::घुसाउ_Two_Way_Joins() {
    // Reset marks
    लागि (V3GraphVertex& v : vertices()) v.user(0);

    तार्किक isSeriesParallel = सत्य;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    लागि (V3GraphVertex& v : vertices()) {
        CfgBlock& bb = static_cast<CfgBlock&>(v);
        // Skip if already visited
        यदि (bb.user()) जारी;
        // Skip if not a branch
        यदि (!bb.isBranch()) जारी;
        // Fix it up, record if failed
        यदि (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = असत्य;
    }

    यदि (m_nEdits != m_nLastOrdered) rpoBlocks();
    यदि (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    फर्काऊ isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

अचर CfgBlock* Cfg_Dominator_रूख::intersect(अचर CfgBlock* ap, अचर CfgBlock* bp) {
    जबसम्म (ap != bp) {
        जबसम्म (*ap > *bp) ap = m_bb2Idom[*ap];
        जबसम्म (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    फर्काऊ ap;
}

Cfg_Dominator_रूख::Cfg_Dominator_रूख(अचर CfgGraph& cfg)
    : m_bb2Idom{cfg.makeBlockMap<अचर CfgBlock*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    लागि (तार्किक changed = सत्य; changed;) {
        changed = असत्य;
        // For each vertex except enter block
        लागि (अचर V3GraphVertex& vtx : cfg.vertices()) {
            अचर CfgBlock& curr = static_cast<अचर CfgBlock&>(vtx);
            यदि (curr.isEnter()) जारी;  // Skip entry block

            // For each predecessor of current block
            अचर CfgBlock* idom = nullptr;
            लागि (अचर V3GraphEdge& edge : curr.inEdges()) {
                अचर CfgBlock& pred = static_cast<अचर CfgBlock&>(*edge.fromp());
                // Skip if perdecessor not yet processed
                यदि (!m_bb2Idom[pred]) जारी;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            यदि (idom == m_bb2Idom[curr]) जारी;
            m_bb2Idom[curr] = idom;
            changed = सत्य;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = nullptr;
}
