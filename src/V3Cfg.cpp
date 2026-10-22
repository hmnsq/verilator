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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Cfg.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3EmitV.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// CfgBlock method definitions

std::string Cfg_ಖಂಡ::name() ಸ್ಥಿರಾಂಕ {
    std::stringstream ss;
    ss << "BB " + std::ಗಮ್ಯ_string(id()) + ":\n";
    ಗಾಗಿ (Ast_ಗಂಟು* nodep : m_stmtps) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstIf* ಸ್ಥಿರಾಂಕ ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debug_Verilog_For_ವೃಕ್ಷ(ifp->condp(), ss);
            ss << ") ...";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstLoopTest* ಸ್ಥಿರಾಂಕ testp = VN_CAST(nodep, LoopTest)) {
            ss << "if  (!";
            V3EmitV::debug_Verilog_For_ವೃಕ್ಷ(testp->condp(), ss);
            ss << ") break;";
        } ಇಲ್ಲದಿದ್ದರೆ {
            V3EmitV::debug_Verilog_For_ವೃಕ್ಷ(nodep, ss);
        }
    }
    std::string ಪಠ್ಯ = VString::replaceSubstr(
        VString::replaceSubstr(ss.str(), "\n", "\\l        "), "\"", "\\\"");
    ಆದರೆ (isEnter()) ಪಠ್ಯ = "**ENTER**\n" + ಪಠ್ಯ;
    ಆದರೆ (isExit()) ಪಠ್ಯ = ಪಠ್ಯ + "\n**EXIT**";
    ಹಿಂತಿರುಗಿ ಪಠ್ಯ;
}
std::string Cfg_ಖಂಡ::dotShape() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ "rect"; }
std::string Cfg_ಖಂಡ::dotRank() ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (isEnter()) ಹಿಂತಿರುಗಿ "source";
    ಆದರೆ (isExit()) ಹಿಂತಿರುಗಿ "sink";
    ಹಿಂತಿರುಗಿ "";
}

//######################################################################
// CfgEdge method definitions

std::string Cfg_ಅಂಚು::dotLabel() ಸ್ಥಿರಾಂಕ {
    std::string label = "E" + std::ಗಮ್ಯ_string(id());
    ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು* ಸ್ಥಿರಾಂಕ untknp = srcp()->untknEdgep();
    ಆದರೆ (ಇದು == untknp) {
        label += " / F";
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (untknp) {
        label += " / T";
    }
    ಹಿಂತಿರುಗಿ label;
}

//######################################################################
// CfgGraph method definitions

ಸ್ಥಿರ ಶೂನ್ಯ cfg_ಕ್ರಮ_ಭೇಟಿ_ಖಂಡ(std::ಸದಿಶ<Cfg_ಖಂಡ*>& post_ಕ್ರಮ_Enumeration, Cfg_ಖಂಡ* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    ಆದರೆ (Cfg_ಖಂಡ* ಸ್ಥಿರಾಂಕ takenp = bbp->takenp()) {
        ಆದರೆ (!takenp->user()) cfg_ಕ್ರಮ_ಭೇಟಿ_ಖಂಡ(post_ಕ್ರಮ_Enumeration, takenp);
        ಆದರೆ (Cfg_ಖಂಡ* ಸ್ಥಿರಾಂಕ untknp = bbp->untknp()) {
            ಆದರೆ (!untknp->user()) cfg_ಕ್ರಮ_ಭೇಟಿ_ಖಂಡ(post_ಕ್ರಮ_Enumeration, untknp);
        }
    }
    // Add to post order enumeration
    post_ಕ್ರಮ_Enumeration.emplace_ಹಿಂಭಾಗ(bbp);
};

ಶೂನ್ಯ Cfg_ನಕ್ಷೆ::rpo_ಖಂಡಗಳು() {
    UASSERT_OBJ(m_nEdits != m_n_ಕೊನೆಯ_Ordered, m_enterp, "Redundant 'CfgGraph::order' call");
    m_n_ಕೊನೆಯ_Ordered = m_nEdits;

    // Reset marks
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::ಸದಿಶ<Cfg_ಖಂಡ*> post_ಕ್ರಮ_Enumeration;
    post_ಕ್ರಮ_Enumeration.reserve(m_n_ಖಂಡಗಳು);
    cfg_ಕ್ರಮ_ಭೇಟಿ_ಖಂಡ(post_ಕ್ರಮ_Enumeration, m_enterp);
    UASSERT_OBJ(post_ಕ್ರಮ_Enumeration.ಗಾತ್ರ_2() == m_n_ಖಂಡಗಳು, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < post_ಕ್ರಮ_Enumeration.ಗಾತ್ರ_2(); ++i) {
        Cfg_ಖಂಡ* ಸ್ಥಿರಾಂಕ bbp = post_ಕ್ರಮ_Enumeration[m_n_ಖಂಡಗಳು - 1 - i];
        bbp->m_rpo_ಸಂಖ್ಯೆ = i;
        vertices().unlink(bbp);
        vertices().link_ಹಿಂಭಾಗ(bbp);
    }

    // Assign edge IDs
    ಗಾತ್ರ_t ಅಂಚು_ಎಣಿಕೆ = 0;
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& e : v.ಹೊರಹೋಗು_ಅಂಚುಗಳು()) static_cast<Cfg_ಅಂಚು&>(e).m_id = ಅಂಚು_ಎಣಿಕೆ++;
    }
    UASSERT_OBJ(ಅಂಚು_ಎಣಿಕೆ == m_n_ಅಂಚುಗಳು, m_enterp, "Inconsistent edge count");
}

ತಾರ್ಕಿಕ Cfg_ನಕ್ಷೆ::ಒಳಗೊಂಡಿದೆ_Loop() ಸ್ಥಿರಾಂಕ {
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : vertices()) {
        ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ& current = static_cast<ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ&>(vtx);
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : current.ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ& successor = *static_cast<ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ*>(ಅಂಚು.ತುದಿ());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            ಆದರೆ (successor.id() < current.id()) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
    }
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
}

ಶೂನ್ಯ Cfg_ನಕ್ಷೆ::minimize() {
    // Remove empty blocks (except enter and exit)
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : vertices().unlinkable()) {
        Cfg_ಖಂಡ* ಸ್ಥಿರಾಂಕ bbp = static_cast<Cfg_ಖಂಡ*>(vtxp);
        ಆದರೆ (bbp->isEnter()) ಮುಂದುವರಿ;
        ಆದರೆ (bbp->isExit()) ಮುಂದುವರಿ;
        ಆದರೆ (!bbp->stmtps().ಖಾಲಿಗಳು()) ಮುಂದುವರಿ;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        Cfg_ಖಂಡ* ಸ್ಥಿರಾಂಕ succp = bbp->takenp();
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep : bbp->ಒಳ_ಅಂಚುಗಳು().unlinkable()) edgep->relink_ತುದಿ(succp);
        ++m_nEdits;
        --m_n_ಅಂಚುಗಳು;
        --m_n_ಖಂಡಗಳು;
        VL_DO_DANGLING(bbp->unlinkDelete(ಇದು), bbp);
    }

    // Combine sequential blocks
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : vertices().unlinkable()) {
        Cfg_ಖಂಡ* ಸ್ಥಿರಾಂಕ srcp = static_cast<Cfg_ಖಂಡ*>(vtxp);
        ಆದರೆ (srcp->isExit()) ಮುಂದುವರಿ;
        ಆದರೆ (srcp->isBranch()) ಮುಂದುವರಿ;
        Cfg_ಖಂಡ* ಸ್ಥಿರಾಂಕ dstp = srcp->takenp();
        ಆದರೆ (dstp->isJoin()) ಮುಂದುವರಿ;
        // Combine them
        ಆದರೆ (srcp->isEnter()) m_enterp = dstp;
        std::ಸದಿಶ<Ast_ಗಂಟು_Stmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.ಗಾತ್ರ_2() + dstp->m_stmtps.ಗಾತ್ರ_2());
        stmtps.ಸೇರಿಸು(stmtps.ಕೊನೆ(), dstp->m_stmtps.begin(), dstp->m_stmtps.ಕೊನೆ());
        dstp->m_stmtps = std::move(stmtps);
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep : srcp->ಒಳ_ಅಂಚುಗಳು().unlinkable()) edgep->relink_ತುದಿ(dstp);
        ++m_nEdits;
        --m_n_ಅಂಚುಗಳು;
        --m_n_ಖಂಡಗಳು;
        VL_DO_DANGLING(srcp->unlinkDelete(ಇದು), srcp);
    }

    ಆದರೆ (m_nEdits != m_n_ಕೊನೆಯ_Ordered) rpo_ಖಂಡಗಳು();
    ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

ಶೂನ್ಯ Cfg_ನಕ್ಷೆ::break_Critical_ಅಂಚುಗಳು() {
    // Gather critical edges
    std::ಸದಿಶ<Cfg_ಅಂಚು*> critical_ಅಂಚುಗಳು;
    critical_ಅಂಚುಗಳು.reserve(m_n_ಅಂಚುಗಳು);
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : vertices()) {
        ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ& bb = static_cast<ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ&>(vtx);
        ಆದರೆ (!bb.isBranch()) ಮುಂದುವರಿ;
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtx.ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ& succ = static_cast<ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ&>(*ಅಂಚು.ತುದಿ());
            ಆದರೆ (!succ.isJoin()) ಮುಂದುವರಿ;
            critical_ಅಂಚುಗಳು.emplace_ಹಿಂಭಾಗ(static_cast<Cfg_ಅಂಚು*>(&ಅಂಚು));
        }
    }
    // Insert blocks
    ಗಾಗಿ (Cfg_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep : critical_ಅಂಚುಗಳು) {
        Cfg_ಖಂಡ* ಸ್ಥಿರಾಂಕ newp = ಕೂಡಿಸು_ಖಂಡ();
        ಕೂಡಿಸು_ತೆಗೆದುಕೊಂಡ_ಅಂಚು(newp, edgep->dstp());
        edgep->relink_ತುದಿ(newp);
    }

    ಆದರೆ (m_nEdits != m_n_ಕೊನೆಯ_Ordered) rpo_ಖಂಡಗಳು();
    ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 9) dumpDotFilePrefixed("cfg-breakCriticalEdges");
}

// Given a branching basic block, if the sub-graph below this branch, up until
// the point where all of its control flow path convertes is series-parallel,
// then return the (potentially newly created) basic block with exactly 2
// predecessors where the two control flow paths from this branch have joined.
// If the relevant sub-graph is not series-parallel (there is a control flow
// path between the branches, or to a path not dominated by the given branch),
// then return nullptr. Cached results in the given map
Cfg_ಖಂಡ* Cfg_ನಕ್ಷೆ::getOrCreateTwoWayJoinFor(Cfg_ಖಂಡ* bbp) {
    UASSERT_OBJ(bbp->isBranch(), bbp, "Not a branch");

    // Mark visited
    UASSERT_OBJ(!bbp->user(), bbp, "Should not visit twice");
    bbp->user(1);

    // We need the edge converting to a join block along both path. This is how we find it:
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ chase_ಅಂಚು = [&](Cfg_ಅಂಚು* edgep) -> Cfg_ಅಂಚು* {
        ಆಗ (ನಿಜ) {
            Cfg_ಖಂಡ* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            ಆದರೆ (dstp->isJoin()) ಹಿಂತಿರುಗಿ edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            ಆಗ (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                ಆದರೆ (!dstp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->ತೆಗೆದುಕೊಂಡ_Edgep();
        }
    };

    // Walk down both paths
    Cfg_ಅಂಚು* ಸ್ಥಿರಾಂಕ ತೆಗೆದುಕೊಂಡ_Edgep = chase_ಅಂಚು(bbp->ತೆಗೆದುಕೊಂಡ_Edgep());
    ಆದರೆ (!ತೆಗೆದುಕೊಂಡ_Edgep) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    Cfg_ಅಂಚು* ಸ್ಥಿರಾಂಕ untknEdgep = chase_ಅಂಚು(bbp->untknEdgep());
    ಆದರೆ (!untknEdgep) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    ಆದರೆ (ತೆಗೆದುಕೊಂಡ_Edgep->dstp() != untknEdgep->dstp()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

    // Pick up the common successor
    Cfg_ಖಂಡ* ಸ್ಥಿರಾಂಕ succp = ತೆಗೆದುಕೊಂಡ_Edgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    ಆದರೆ (succp->isTwoWayJoin()) ಹಿಂತಿರುಗಿ succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    Cfg_ಖಂಡ* ಸ್ಥಿರಾಂಕ joinp = ಕೂಡಿಸು_ಖಂಡ();
    ಕೂಡಿಸು_ತೆಗೆದುಕೊಂಡ_ಅಂಚು(joinp, succp);
    ತೆಗೆದುಕೊಂಡ_Edgep->relink_ತುದಿ(joinp);
    untknEdgep->relink_ತುದಿ(joinp);
    ಹಿಂತಿರುಗಿ joinp;
}

ತಾರ್ಕಿಕ Cfg_ನಕ್ಷೆ::ಸೇರಿಸು_Two_Way_Joins() {
    // Reset marks
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& v : vertices()) v.user(0);

    ತಾರ್ಕಿಕ isSeriesParallel = ನಿಜ;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& v : vertices()) {
        Cfg_ಖಂಡ& bb = static_cast<Cfg_ಖಂಡ&>(v);
        // Skip if already visited
        ಆದರೆ (bb.user()) ಮುಂದುವರಿ;
        // Skip if not a branch
        ಆದರೆ (!bb.isBranch()) ಮುಂದುವರಿ;
        // Fix it up, record if failed
        ಆದರೆ (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = ಸುಳ್ಳು;
    }

    ಆದರೆ (m_nEdits != m_n_ಕೊನೆಯ_Ordered) rpo_ಖಂಡಗಳು();
    ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    ಹಿಂತಿರುಗಿ isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* Cfg_Dominator_ವೃಕ್ಷ::intersect(ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* ap, ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* bp) {
    ಆಗ (ap != bp) {
        ಆಗ (*ap > *bp) ap = m_bb2Idom[*ap];
        ಆಗ (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    ಹಿಂತಿರುಗಿ ap;
}

Cfg_Dominator_ವೃಕ್ಷ::Cfg_Dominator_ವೃಕ್ಷ(ಸ್ಥಿರಾಂಕ Cfg_ನಕ್ಷೆ& cfg)
    : m_bb2Idom{cfg.make_ಖಂಡ_Map<ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    ಗಾಗಿ (ತಾರ್ಕಿಕ ಬದಲಾಯಿತು = ನಿಜ; ಬದಲಾಯಿತು;) {
        ಬದಲಾಯಿತು = ಸುಳ್ಳು;
        // For each vertex except enter block
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : cfg.vertices()) {
            ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ& curr = static_cast<ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ&>(vtx);
            ಆದರೆ (curr.isEnter()) ಮುಂದುವರಿ;  // Skip entry block

            // For each predecessor of current block
            ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* idom = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : curr.ಒಳ_ಅಂಚುಗಳು()) {
                ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ& pred = static_cast<ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ&>(*ಅಂಚು.fromp());
                // Skip if perdecessor not yet processed
                ಆದರೆ (!m_bb2Idom[pred]) ಮುಂದುವರಿ;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            ಆದರೆ (idom == m_bb2Idom[curr]) ಮುಂದುವರಿ;
            m_bb2Idom[curr] = idom;
            ಬದಲಾಯಿತು = ನಿಜ;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = ಶೂನ್ಯಸೂಚಕ;
}
