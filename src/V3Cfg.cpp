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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Cfg.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitV.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// CfgBlock method definitions

std::string CfgBlock::name() ଅଚଳ {
    std::stringstream ss;
    ss << "BB " + std::to_string(id()) + ":\n";
    ପାଇଁ (Ast_ଗଣ୍ଠି* nodep : m_stmtps) {
        ଯଦି (ଅଚଳ AstIf* ଅଚଳ ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debug_Verilog_For_ବୃକ୍ଷ(ifp->condp(), ss);
            ss << ") ...";
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstLoopTest* ଅଚଳ testp = VN_CAST(nodep, LoopTest)) {
            ss << "if  (!";
            V3EmitV::debug_Verilog_For_ବୃକ୍ଷ(testp->condp(), ss);
            ss << ") break;";
        } ଅନ୍ୟଥା {
            V3EmitV::debug_Verilog_For_ବୃକ୍ଷ(nodep, ss);
        }
    }
    std::string text = VString::replaceSubstr(
        VString::replaceSubstr(ss.str(), "\n", "\\l        "), "\"", "\\\"");
    ଯଦି (isEnter()) text = "**ENTER**\n" + text;
    ଯଦି (isExit()) text = text + "\n**EXIT**";
    ଫେରନ୍ତୁ text;
}
std::string CfgBlock::dotShape() ଅଚଳ { ଫେରନ୍ତୁ "rect"; }
std::string CfgBlock::dotRank() ଅଚଳ {
    ଯଦି (isEnter()) ଫେରନ୍ତୁ "source";
    ଯଦି (isExit()) ଫେରନ୍ତୁ "sink";
    ଫେରନ୍ତୁ "";
}

//######################################################################
// CfgEdge method definitions

std::string CfgEdge::dotLabel() ଅଚଳ {
    std::string label = "E" + std::to_string(id());
    ଅଚଳ CfgEdge* ଅଚଳ untknp = srcp()->untknEdgep();
    ଯଦି (ଏହି == untknp) {
        label += " / F";
    } ଅନ୍ୟଥା ଯଦି (untknp) {
        label += " / T";
    }
    ଫେରନ୍ତୁ label;
}

//######################################################################
// CfgGraph method definitions

ସ୍ଥିର ଶୂନ୍ୟ cfgOrderVisitBlock(std::vector<CfgBlock*>& postOrderEnumeration, CfgBlock* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    ଯଦି (CfgBlock* ଅଚଳ takenp = bbp->takenp()) {
        ଯଦି (!takenp->user()) cfgOrderVisitBlock(postOrderEnumeration, takenp);
        ଯଦି (CfgBlock* ଅଚଳ untknp = bbp->untknp()) {
            ଯଦି (!untknp->user()) cfgOrderVisitBlock(postOrderEnumeration, untknp);
        }
    }
    // Add to post order enumeration
    postOrderEnumeration.emplace_back(bbp);
};

ଶୂନ୍ୟ CfgGraph::rpoBlocks() {
    UASSERT_OBJ(m_nEdits != m_nLastOrdered, m_enterp, "Redundant 'CfgGraph::order' call");
    m_nLastOrdered = m_nEdits;

    // Reset marks
    ପାଇଁ (V3GraphVertex& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::vector<CfgBlock*> postOrderEnumeration;
    postOrderEnumeration.reserve(m_nBlocks);
    cfgOrderVisitBlock(postOrderEnumeration, m_enterp);
    UASSERT_OBJ(postOrderEnumeration.size() == m_nBlocks, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    ପାଇଁ (size_t i = 0; i < postOrderEnumeration.size(); ++i) {
        CfgBlock* ଅଚଳ bbp = postOrderEnumeration[m_nBlocks - 1 - i];
        bbp->m_rpo_ସଂଖ୍ୟା = i;
        vertices().unlink(bbp);
        vertices().linkBack(bbp);
    }

    // Assign edge IDs
    size_t edge_ଗଣନା = 0;
    ପାଇଁ (V3GraphVertex& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        ପାଇଁ (V3GraphEdge& e : v.outEdges()) static_cast<CfgEdge&>(e).m_id = edge_ଗଣନା++;
    }
    UASSERT_OBJ(edge_ଗଣନା == m_nEdges, m_enterp, "Inconsistent edge count");
}

ତାର୍କିକ CfgGraph::containsLoop() ଅଚଳ {
    ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : vertices()) {
        ଅଚଳ CfgBlock& current = static_cast<ଅଚଳ CfgBlock&>(vtx);
        ପାଇଁ (ଅଚଳ V3GraphEdge& edge : current.outEdges()) {
            ଅଚଳ CfgBlock& successor = *static_cast<ଅଚଳ CfgBlock*>(edge.ଶୀର୍ଷ());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            ଯଦି (successor.id() < current.id()) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
    }
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
}

ଶୂନ୍ୟ CfgGraph::minimize() {
    // Remove empty blocks (except enter and exit)
    ପାଇଁ (V3GraphVertex* ଅଚଳ vtxp : vertices().unlinkable()) {
        CfgBlock* ଅଚଳ bbp = static_cast<CfgBlock*>(vtxp);
        ଯଦି (bbp->isEnter()) ଜାରି;
        ଯଦି (bbp->isExit()) ଜାରି;
        ଯଦି (!bbp->stmtps().empty()) ଜାରି;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        CfgBlock* ଅଚଳ succp = bbp->takenp();
        ପାଇଁ (V3GraphEdge* ଅଚଳ edgep : bbp->inEdges().unlinkable()) edgep->relink_ଶୀର୍ଷ(succp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(bbp->unlinkDelete(ଏହି), bbp);
    }

    // Combine sequential blocks
    ପାଇଁ (V3GraphVertex* ଅଚଳ vtxp : vertices().unlinkable()) {
        CfgBlock* ଅଚଳ srcp = static_cast<CfgBlock*>(vtxp);
        ଯଦି (srcp->isExit()) ଜାରି;
        ଯଦି (srcp->isBranch()) ଜାରି;
        CfgBlock* ଅଚଳ dstp = srcp->takenp();
        ଯଦି (dstp->isJoin()) ଜାରି;
        // Combine them
        ଯଦି (srcp->isEnter()) m_enterp = dstp;
        std::vector<Ast_ଗଣ୍ଠି_Stmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.size() + dstp->m_stmtps.size());
        stmtps.ଭର୍ତ୍ତି(stmtps.end(), dstp->m_stmtps.begin(), dstp->m_stmtps.end());
        dstp->m_stmtps = std::move(stmtps);
        ପାଇଁ (V3GraphEdge* ଅଚଳ edgep : srcp->inEdges().unlinkable()) edgep->relink_ଶୀର୍ଷ(dstp);
        ++m_nEdits;
        --m_nEdges;
        --m_nBlocks;
        VL_DO_DANGLING(srcp->unlinkDelete(ଏହି), srcp);
    }

    ଯଦି (m_nEdits != m_nLastOrdered) rpoBlocks();
    ଯଦି (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

ଶୂନ୍ୟ CfgGraph::breakCriticalEdges() {
    // Gather critical edges
    std::vector<CfgEdge*> criticalEdges;
    criticalEdges.reserve(m_nEdges);
    ପାଇଁ (V3GraphVertex& vtx : vertices()) {
        ଅଚଳ CfgBlock& bb = static_cast<ଅଚଳ CfgBlock&>(vtx);
        ଯଦି (!bb.isBranch()) ଜାରି;
        ପାଇଁ (V3GraphEdge& edge : vtx.outEdges()) {
            ଅଚଳ CfgBlock& succ = static_cast<ଅଚଳ CfgBlock&>(*edge.ଶୀର୍ଷ());
            ଯଦି (!succ.isJoin()) ଜାରି;
            criticalEdges.emplace_back(static_cast<CfgEdge*>(&edge));
        }
    }
    // Insert blocks
    ପାଇଁ (CfgEdge* ଅଚଳ edgep : criticalEdges) {
        CfgBlock* ଅଚଳ newp = addBlock();
        addTakenEdge(newp, edgep->dstp());
        edgep->relink_ଶୀର୍ଷ(newp);
    }

    ଯଦି (m_nEdits != m_nLastOrdered) rpoBlocks();
    ଯଦି (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-breakCriticalEdges");
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
    ଅଚଳ auto chaseEdge = [&](CfgEdge* edgep) -> CfgEdge* {
        ଯେତେବେଳେ (ସତ୍ୟ) {
            CfgBlock* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            ଯଦି (dstp->isJoin()) ଫେରନ୍ତୁ edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            ଯେତେବେଳେ (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                ଯଦି (!dstp) ଫେରନ୍ତୁ nullptr;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->takenEdgep();
        }
    };

    // Walk down both paths
    CfgEdge* ଅଚଳ takenEdgep = chaseEdge(bbp->takenEdgep());
    ଯଦି (!takenEdgep) ଫେରନ୍ତୁ nullptr;
    CfgEdge* ଅଚଳ untknEdgep = chaseEdge(bbp->untknEdgep());
    ଯଦି (!untknEdgep) ଫେରନ୍ତୁ nullptr;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    ଯଦି (takenEdgep->dstp() != untknEdgep->dstp()) ଫେରନ୍ତୁ nullptr;

    // Pick up the common successor
    CfgBlock* ଅଚଳ succp = takenEdgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    ଯଦି (succp->isTwoWayJoin()) ଫେରନ୍ତୁ succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    CfgBlock* ଅଚଳ joinp = addBlock();
    addTakenEdge(joinp, succp);
    takenEdgep->relink_ଶୀର୍ଷ(joinp);
    untknEdgep->relink_ଶୀର୍ଷ(joinp);
    ଫେରନ୍ତୁ joinp;
}

ତାର୍କିକ CfgGraph::ଭର୍ତ୍ତି_Two_Way_Joins() {
    // Reset marks
    ପାଇଁ (V3GraphVertex& v : vertices()) v.user(0);

    ତାର୍କିକ isSeriesParallel = ସତ୍ୟ;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    ପାଇଁ (V3GraphVertex& v : vertices()) {
        CfgBlock& bb = static_cast<CfgBlock&>(v);
        // Skip if already visited
        ଯଦି (bb.user()) ଜାରି;
        // Skip if not a branch
        ଯଦି (!bb.isBranch()) ଜାରି;
        // Fix it up, record if failed
        ଯଦି (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = ମିଥ୍ୟା;
    }

    ଯଦି (m_nEdits != m_nLastOrdered) rpoBlocks();
    ଯଦି (dumpGraphLevel() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    ଫେରନ୍ତୁ isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

ଅଚଳ CfgBlock* Cfg_Dominator_ବୃକ୍ଷ::intersect(ଅଚଳ CfgBlock* ap, ଅଚଳ CfgBlock* bp) {
    ଯେତେବେଳେ (ap != bp) {
        ଯେତେବେଳେ (*ap > *bp) ap = m_bb2Idom[*ap];
        ଯେତେବେଳେ (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    ଫେରନ୍ତୁ ap;
}

Cfg_Dominator_ବୃକ୍ଷ::Cfg_Dominator_ବୃକ୍ଷ(ଅଚଳ CfgGraph& cfg)
    : m_bb2Idom{cfg.makeBlockMap<ଅଚଳ CfgBlock*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    ପାଇଁ (ତାର୍କିକ changed = ସତ୍ୟ; changed;) {
        changed = ମିଥ୍ୟା;
        // For each vertex except enter block
        ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : cfg.vertices()) {
            ଅଚଳ CfgBlock& curr = static_cast<ଅଚଳ CfgBlock&>(vtx);
            ଯଦି (curr.isEnter()) ଜାରି;  // Skip entry block

            // For each predecessor of current block
            ଅଚଳ CfgBlock* idom = nullptr;
            ପାଇଁ (ଅଚଳ V3GraphEdge& edge : curr.inEdges()) {
                ଅଚଳ CfgBlock& pred = static_cast<ଅଚଳ CfgBlock&>(*edge.fromp());
                // Skip if perdecessor not yet processed
                ଯଦି (!m_bb2Idom[pred]) ଜାରି;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            ଯଦି (idom == m_bb2Idom[curr]) ଜାରି;
            m_bb2Idom[curr] = idom;
            changed = ସତ୍ୟ;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = nullptr;
}
