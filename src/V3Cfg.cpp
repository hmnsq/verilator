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

std::string Cfg_بلاڪ::name() ثابت {
    std::stringstream ss;
    ss << "BB " + std::منزل_string(id()) + ":\n";
    لكل (Ast_ڳنڍ* nodep : m_stmtps) {
        إذا (ثابت AstIf* ثابت ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debug_Verilog_For_وڻ(ifp->condp(), ss);
            ss << ") ...";
        } وإلا إذا (ثابت AstLoopTest* ثابت testp = VN_CAST(nodep, LoopTest)) {
            ss << "if  (!";
            V3EmitV::debug_Verilog_For_وڻ(testp->condp(), ss);
            ss << ") break;";
        } وإلا {
            V3EmitV::debug_Verilog_For_وڻ(nodep, ss);
        }
    }
    std::string متن = VString::replaceSubstr(
        VString::replaceSubstr(ss.str(), "\n", "\\l        "), "\"", "\\\"");
    إذا (isEnter()) متن = "**ENTER**\n" + متن;
    إذا (isExit()) متن = متن + "\n**EXIT**";
    أرجع متن;
}
std::string Cfg_بلاڪ::dotShape() ثابت { أرجع "rect"; }
std::string Cfg_بلاڪ::dotRank() ثابت {
    إذا (isEnter()) أرجع "source";
    إذا (isExit()) أرجع "sink";
    أرجع "";
}

//######################################################################
// CfgEdge method definitions

std::string Cfg_ڪنارو::dotLabel() ثابت {
    std::string label = "E" + std::منزل_string(id());
    ثابت Cfg_ڪنارو* ثابت untknp = srcp()->untknEdgep();
    إذا (هذا == untknp) {
        label += " / F";
    } وإلا إذا (untknp) {
        label += " / T";
    }
    أرجع label;
}

//######################################################################
// CfgGraph method definitions

ساكن فراغ cfg_ترتيب_گهمو_بلاڪ(std::ويڪٽر<Cfg_بلاڪ*>& post_ترتيب_Enumeration, Cfg_بلاڪ* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    إذا (Cfg_بلاڪ* ثابت takenp = bbp->takenp()) {
        إذا (!takenp->user()) cfg_ترتيب_گهمو_بلاڪ(post_ترتيب_Enumeration, takenp);
        إذا (Cfg_بلاڪ* ثابت untknp = bbp->untknp()) {
            إذا (!untknp->user()) cfg_ترتيب_گهمو_بلاڪ(post_ترتيب_Enumeration, untknp);
        }
    }
    // Add to post order enumeration
    post_ترتيب_Enumeration.emplace_پٺ(bbp);
};

فراغ Cfg_گراف::rpo_بلاڪون() {
    UASSERT_OBJ(m_nEdits != m_n_آخري_Ordered, m_enterp, "Redundant 'CfgGraph::order' call");
    m_n_آخري_Ordered = m_nEdits;

    // Reset marks
    لكل (V3_گراف_راس& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::ويڪٽر<Cfg_بلاڪ*> post_ترتيب_Enumeration;
    post_ترتيب_Enumeration.reserve(m_n_بلاڪون);
    cfg_ترتيب_گهمو_بلاڪ(post_ترتيب_Enumeration, m_enterp);
    UASSERT_OBJ(post_ترتيب_Enumeration.سائيز() == m_n_بلاڪون, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    لكل (سائيز_t i = 0; i < post_ترتيب_Enumeration.سائيز(); ++i) {
        Cfg_بلاڪ* ثابت bbp = post_ترتيب_Enumeration[m_n_بلاڪون - 1 - i];
        bbp->m_rpo_عدد = i;
        vertices().unlink(bbp);
        vertices().link_پٺ(bbp);
    }

    // Assign edge IDs
    سائيز_t ڪنارو_ڳڻپ = 0;
    لكل (V3_گراف_راس& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        لكل (V3_گراف_ڪنارو& e : v.ٻاهريون_ڪنارا()) static_cast<Cfg_ڪنارو&>(e).m_id = ڪنارو_ڳڻپ++;
    }
    UASSERT_OBJ(ڪنارو_ڳڻپ == m_n_ڪنارا, m_enterp, "Inconsistent edge count");
}

منطقي Cfg_گراف::شامل_آهي_Loop() ثابت {
    لكل (ثابت V3_گراف_راس& vtx : vertices()) {
        ثابت Cfg_بلاڪ& current = static_cast<ثابت Cfg_بلاڪ&>(vtx);
        لكل (ثابت V3_گراف_ڪنارو& ڪنارو : current.ٻاهريون_ڪنارا()) {
            ثابت Cfg_بلاڪ& successor = *static_cast<ثابت Cfg_بلاڪ*>(ڪنارو.چوٽي());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            إذا (successor.id() < current.id()) أرجع سچ;
        }
    }
    أرجع خطأ;
}

فراغ Cfg_گراف::minimize() {
    // Remove empty blocks (except enter and exit)
    لكل (V3_گراف_راس* ثابت vtxp : vertices().unlinkable()) {
        Cfg_بلاڪ* ثابت bbp = static_cast<Cfg_بلاڪ*>(vtxp);
        إذا (bbp->isEnter()) استمر;
        إذا (bbp->isExit()) استمر;
        إذا (!bbp->stmtps().خالي_جايون()) استمر;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        Cfg_بلاڪ* ثابت succp = bbp->takenp();
        لكل (V3_گراف_ڪنارو* ثابت edgep : bbp->اندريون_ڪنارا().unlinkable()) edgep->relink_چوٽي(succp);
        ++m_nEdits;
        --m_n_ڪنارا;
        --m_n_بلاڪون;
        VL_DO_DANGLING(bbp->unlinkDelete(هذا), bbp);
    }

    // Combine sequential blocks
    لكل (V3_گراف_راس* ثابت vtxp : vertices().unlinkable()) {
        Cfg_بلاڪ* ثابت srcp = static_cast<Cfg_بلاڪ*>(vtxp);
        إذا (srcp->isExit()) استمر;
        إذا (srcp->isBranch()) استمر;
        Cfg_بلاڪ* ثابت dstp = srcp->takenp();
        إذا (dstp->isJoin()) استمر;
        // Combine them
        إذا (srcp->isEnter()) m_enterp = dstp;
        std::ويڪٽر<Ast_ڳنڍ_Stmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.سائيز() + dstp->m_stmtps.سائيز());
        stmtps.وجهو(stmtps.پڇاڙي(), dstp->m_stmtps.begin(), dstp->m_stmtps.پڇاڙي());
        dstp->m_stmtps = std::move(stmtps);
        لكل (V3_گراف_ڪنارو* ثابت edgep : srcp->اندريون_ڪنارا().unlinkable()) edgep->relink_چوٽي(dstp);
        ++m_nEdits;
        --m_n_ڪنارا;
        --m_n_بلاڪون;
        VL_DO_DANGLING(srcp->unlinkDelete(هذا), srcp);
    }

    إذا (m_nEdits != m_n_آخري_Ordered) rpo_بلاڪون();
    إذا (dump_گراف_Level() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

فراغ Cfg_گراف::break_Critical_ڪنارا() {
    // Gather critical edges
    std::ويڪٽر<Cfg_ڪنارو*> critical_ڪنارا;
    critical_ڪنارا.reserve(m_n_ڪنارا);
    لكل (V3_گراف_راس& vtx : vertices()) {
        ثابت Cfg_بلاڪ& bb = static_cast<ثابت Cfg_بلاڪ&>(vtx);
        إذا (!bb.isBranch()) استمر;
        لكل (V3_گراف_ڪنارو& ڪنارو : vtx.ٻاهريون_ڪنارا()) {
            ثابت Cfg_بلاڪ& succ = static_cast<ثابت Cfg_بلاڪ&>(*ڪنارو.چوٽي());
            إذا (!succ.isJoin()) استمر;
            critical_ڪنارا.emplace_پٺ(static_cast<Cfg_ڪنارو*>(&ڪنارو));
        }
    }
    // Insert blocks
    لكل (Cfg_ڪنارو* ثابت edgep : critical_ڪنارا) {
        Cfg_بلاڪ* ثابت newp = جوڙيو_بلاڪ();
        جوڙيو_ورتل_ڪنارو(newp, edgep->dstp());
        edgep->relink_چوٽي(newp);
    }

    إذا (m_nEdits != m_n_آخري_Ordered) rpo_بلاڪون();
    إذا (dump_گراف_Level() >= 9) dumpDotFilePrefixed("cfg-breakCriticalEdges");
}

// Given a branching basic block, if the sub-graph below this branch, up until
// the point where all of its control flow path convertes is series-parallel,
// then return the (potentially newly created) basic block with exactly 2
// predecessors where the two control flow paths from this branch have joined.
// If the relevant sub-graph is not series-parallel (there is a control flow
// path between the branches, or to a path not dominated by the given branch),
// then return nullptr. Cached results in the given map
Cfg_بلاڪ* Cfg_گراف::getOrCreateTwoWayJoinFor(Cfg_بلاڪ* bbp) {
    UASSERT_OBJ(bbp->isBranch(), bbp, "Not a branch");

    // Mark visited
    UASSERT_OBJ(!bbp->user(), bbp, "Should not visit twice");
    bbp->user(1);

    // We need the edge converting to a join block along both path. This is how we find it:
    ثابت خودڪار chase_ڪنارو = [&](Cfg_ڪنارو* edgep) -> Cfg_ڪنارو* {
        طالما (سچ) {
            Cfg_بلاڪ* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            إذا (dstp->isJoin()) أرجع edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            طالما (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                إذا (!dstp) أرجع خالي_اشارو;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->ورتل_Edgep();
        }
    };

    // Walk down both paths
    Cfg_ڪنارو* ثابت ورتل_Edgep = chase_ڪنارو(bbp->ورتل_Edgep());
    إذا (!ورتل_Edgep) أرجع خالي_اشارو;
    Cfg_ڪنارو* ثابت untknEdgep = chase_ڪنارو(bbp->untknEdgep());
    إذا (!untknEdgep) أرجع خالي_اشارو;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    إذا (ورتل_Edgep->dstp() != untknEdgep->dstp()) أرجع خالي_اشارو;

    // Pick up the common successor
    Cfg_بلاڪ* ثابت succp = ورتل_Edgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    إذا (succp->isTwoWayJoin()) أرجع succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    Cfg_بلاڪ* ثابت joinp = جوڙيو_بلاڪ();
    جوڙيو_ورتل_ڪنارو(joinp, succp);
    ورتل_Edgep->relink_چوٽي(joinp);
    untknEdgep->relink_چوٽي(joinp);
    أرجع joinp;
}

منطقي Cfg_گراف::وجهو_Two_Way_Joins() {
    // Reset marks
    لكل (V3_گراف_راس& v : vertices()) v.user(0);

    منطقي isSeriesParallel = سچ;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    لكل (V3_گراف_راس& v : vertices()) {
        Cfg_بلاڪ& bb = static_cast<Cfg_بلاڪ&>(v);
        // Skip if already visited
        إذا (bb.user()) استمر;
        // Skip if not a branch
        إذا (!bb.isBranch()) استمر;
        // Fix it up, record if failed
        إذا (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = خطأ;
    }

    إذا (m_nEdits != m_n_آخري_Ordered) rpo_بلاڪون();
    إذا (dump_گراف_Level() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    أرجع isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

ثابت Cfg_بلاڪ* Cfg_Dominator_وڻ::intersect(ثابت Cfg_بلاڪ* ap, ثابت Cfg_بلاڪ* bp) {
    طالما (ap != bp) {
        طالما (*ap > *bp) ap = m_bb2Idom[*ap];
        طالما (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    أرجع ap;
}

Cfg_Dominator_وڻ::Cfg_Dominator_وڻ(ثابت Cfg_گراف& cfg)
    : m_bb2Idom{cfg.make_بلاڪ_Map<ثابت Cfg_بلاڪ*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    لكل (منطقي بدليو = سچ; بدليو;) {
        بدليو = خطأ;
        // For each vertex except enter block
        لكل (ثابت V3_گراف_راس& vtx : cfg.vertices()) {
            ثابت Cfg_بلاڪ& curr = static_cast<ثابت Cfg_بلاڪ&>(vtx);
            إذا (curr.isEnter()) استمر;  // Skip entry block

            // For each predecessor of current block
            ثابت Cfg_بلاڪ* idom = خالي_اشارو;
            لكل (ثابت V3_گراف_ڪنارو& ڪنارو : curr.اندريون_ڪنارا()) {
                ثابت Cfg_بلاڪ& pred = static_cast<ثابت Cfg_بلاڪ&>(*ڪنارو.fromp());
                // Skip if perdecessor not yet processed
                إذا (!m_bb2Idom[pred]) استمر;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            إذا (idom == m_bb2Idom[curr]) استمر;
            m_bb2Idom[curr] = idom;
            بدليو = سچ;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = خالي_اشارو;
}
