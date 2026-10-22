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

std::string Cfg_بلوک::name() ثابت {
    std::stringstream ss;
    ss << "BB " + std::مقصد_string(id()) + ":\n";
    لكل (Ast_گره* nodep : m_stmtps) {
        إذا (ثابت AstIf* ثابت ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debug_Verilog_For_درخت(ifp->condp(), ss);
            ss << ") ...";
        } وإلا إذا (ثابت AstLoopTest* ثابت testp = VN_CAST(nodep, LoopTest)) {
            ss << "if  (!";
            V3EmitV::debug_Verilog_For_درخت(testp->condp(), ss);
            ss << ") break;";
        } وإلا {
            V3EmitV::debug_Verilog_For_درخت(nodep, ss);
        }
    }
    std::string متن = VString::replaceSubstr(
        VString::replaceSubstr(ss.str(), "\n", "\\l        "), "\"", "\\\"");
    إذا (isEnter()) متن = "**ENTER**\n" + متن;
    إذا (isExit()) متن = متن + "\n**EXIT**";
    أرجع متن;
}
std::string Cfg_بلوک::dotShape() ثابت { أرجع "rect"; }
std::string Cfg_بلوک::dotRank() ثابت {
    إذا (isEnter()) أرجع "source";
    إذا (isExit()) أرجع "sink";
    أرجع "";
}

//######################################################################
// CfgEdge method definitions

std::string Cfg_یال::dotLabel() ثابت {
    std::string label = "E" + std::مقصد_string(id());
    ثابت Cfg_یال* ثابت untknp = srcp()->untknEdgep();
    إذا (هذا == untknp) {
        label += " / F";
    } وإلا إذا (untknp) {
        label += " / T";
    }
    أرجع label;
}

//######################################################################
// CfgGraph method definitions

ساكن فراغ cfg_ترتیب_بازدید_بلوک(std::وکتور<Cfg_بلوک*>& post_ترتیب_Enumeration, Cfg_بلوک* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    إذا (Cfg_بلوک* ثابت takenp = bbp->takenp()) {
        إذا (!takenp->user()) cfg_ترتیب_بازدید_بلوک(post_ترتیب_Enumeration, takenp);
        إذا (Cfg_بلوک* ثابت untknp = bbp->untknp()) {
            إذا (!untknp->user()) cfg_ترتیب_بازدید_بلوک(post_ترتیب_Enumeration, untknp);
        }
    }
    // Add to post order enumeration
    post_ترتیب_Enumeration.emplace_پشت(bbp);
};

فراغ Cfg_گراف::rpo_بلوکها() {
    UASSERT_OBJ(m_nEdits != m_n_آخرین_Ordered, m_enterp, "Redundant 'CfgGraph::order' call");
    m_n_آخرین_Ordered = m_nEdits;

    // Reset marks
    لكل (V3_گراف_رأس& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::وکتور<Cfg_بلوک*> post_ترتیب_Enumeration;
    post_ترتیب_Enumeration.reserve(m_n_بلوکها);
    cfg_ترتیب_بازدید_بلوک(post_ترتیب_Enumeration, m_enterp);
    UASSERT_OBJ(post_ترتیب_Enumeration.اندازه() == m_n_بلوکها, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    لكل (اندازه_t i = 0; i < post_ترتیب_Enumeration.اندازه(); ++i) {
        Cfg_بلوک* ثابت bbp = post_ترتیب_Enumeration[m_n_بلوکها - 1 - i];
        bbp->m_rpo_عدد = i;
        vertices().unlink(bbp);
        vertices().link_پشت(bbp);
    }

    // Assign edge IDs
    اندازه_t یال_شمار = 0;
    لكل (V3_گراف_رأس& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        لكل (V3_گراف_یال& e : v.بیرون_یالها()) static_cast<Cfg_یال&>(e).m_id = یال_شمار++;
    }
    UASSERT_OBJ(یال_شمار == m_n_یالها, m_enterp, "Inconsistent edge count");
}

منطقي Cfg_گراف::شامل_است_Loop() ثابت {
    لكل (ثابت V3_گراف_رأس& vtx : vertices()) {
        ثابت Cfg_بلوک& current = static_cast<ثابت Cfg_بلوک&>(vtx);
        لكل (ثابت V3_گراف_یال& یال : current.بیرون_یالها()) {
            ثابت Cfg_بلوک& successor = *static_cast<ثابت Cfg_بلوک*>(یال.بالاترین());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            إذا (successor.id() < current.id()) أرجع درست;
        }
    }
    أرجع خطأ;
}

فراغ Cfg_گراف::minimize() {
    // Remove empty blocks (except enter and exit)
    لكل (V3_گراف_رأس* ثابت vtxp : vertices().unlinkable()) {
        Cfg_بلوک* ثابت bbp = static_cast<Cfg_بلوک*>(vtxp);
        إذا (bbp->isEnter()) استمر;
        إذا (bbp->isExit()) استمر;
        إذا (!bbp->stmtps().خالی()) استمر;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        Cfg_بلوک* ثابت succp = bbp->takenp();
        لكل (V3_گراف_یال* ثابت edgep : bbp->درون_یالها().unlinkable()) edgep->relink_بالاترین(succp);
        ++m_nEdits;
        --m_n_یالها;
        --m_n_بلوکها;
        VL_DO_DANGLING(bbp->unlinkDelete(هذا), bbp);
    }

    // Combine sequential blocks
    لكل (V3_گراف_رأس* ثابت vtxp : vertices().unlinkable()) {
        Cfg_بلوک* ثابت srcp = static_cast<Cfg_بلوک*>(vtxp);
        إذا (srcp->isExit()) استمر;
        إذا (srcp->isBranch()) استمر;
        Cfg_بلوک* ثابت dstp = srcp->takenp();
        إذا (dstp->isJoin()) استمر;
        // Combine them
        إذا (srcp->isEnter()) m_enterp = dstp;
        std::وکتور<Ast_گره_Stmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.اندازه() + dstp->m_stmtps.اندازه());
        stmtps.درج(stmtps.انتها(), dstp->m_stmtps.begin(), dstp->m_stmtps.انتها());
        dstp->m_stmtps = std::move(stmtps);
        لكل (V3_گراف_یال* ثابت edgep : srcp->درون_یالها().unlinkable()) edgep->relink_بالاترین(dstp);
        ++m_nEdits;
        --m_n_یالها;
        --m_n_بلوکها;
        VL_DO_DANGLING(srcp->unlinkDelete(هذا), srcp);
    }

    إذا (m_nEdits != m_n_آخرین_Ordered) rpo_بلوکها();
    إذا (dump_گراف_Level() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

فراغ Cfg_گراف::break_Critical_یالها() {
    // Gather critical edges
    std::وکتور<Cfg_یال*> critical_یالها;
    critical_یالها.reserve(m_n_یالها);
    لكل (V3_گراف_رأس& vtx : vertices()) {
        ثابت Cfg_بلوک& bb = static_cast<ثابت Cfg_بلوک&>(vtx);
        إذا (!bb.isBranch()) استمر;
        لكل (V3_گراف_یال& یال : vtx.بیرون_یالها()) {
            ثابت Cfg_بلوک& succ = static_cast<ثابت Cfg_بلوک&>(*یال.بالاترین());
            إذا (!succ.isJoin()) استمر;
            critical_یالها.emplace_پشت(static_cast<Cfg_یال*>(&یال));
        }
    }
    // Insert blocks
    لكل (Cfg_یال* ثابت edgep : critical_یالها) {
        Cfg_بلوک* ثابت newp = جمع_کن_بلوک();
        جمع_کن_گرفته_یال(newp, edgep->dstp());
        edgep->relink_بالاترین(newp);
    }

    إذا (m_nEdits != m_n_آخرین_Ordered) rpo_بلوکها();
    إذا (dump_گراف_Level() >= 9) dumpDotFilePrefixed("cfg-breakCriticalEdges");
}

// Given a branching basic block, if the sub-graph below this branch, up until
// the point where all of its control flow path convertes is series-parallel,
// then return the (potentially newly created) basic block with exactly 2
// predecessors where the two control flow paths from this branch have joined.
// If the relevant sub-graph is not series-parallel (there is a control flow
// path between the branches, or to a path not dominated by the given branch),
// then return nullptr. Cached results in the given map
Cfg_بلوک* Cfg_گراف::getOrCreateTwoWayJoinFor(Cfg_بلوک* bbp) {
    UASSERT_OBJ(bbp->isBranch(), bbp, "Not a branch");

    // Mark visited
    UASSERT_OBJ(!bbp->user(), bbp, "Should not visit twice");
    bbp->user(1);

    // We need the edge converting to a join block along both path. This is how we find it:
    ثابت خودکار chase_یال = [&](Cfg_یال* edgep) -> Cfg_یال* {
        طالما (درست) {
            Cfg_بلوک* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            إذا (dstp->isJoin()) أرجع edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            طالما (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                إذا (!dstp) أرجع اشاره_تهی;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->گرفته_Edgep();
        }
    };

    // Walk down both paths
    Cfg_یال* ثابت گرفته_Edgep = chase_یال(bbp->گرفته_Edgep());
    إذا (!گرفته_Edgep) أرجع اشاره_تهی;
    Cfg_یال* ثابت untknEdgep = chase_یال(bbp->untknEdgep());
    إذا (!untknEdgep) أرجع اشاره_تهی;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    إذا (گرفته_Edgep->dstp() != untknEdgep->dstp()) أرجع اشاره_تهی;

    // Pick up the common successor
    Cfg_بلوک* ثابت succp = گرفته_Edgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    إذا (succp->isTwoWayJoin()) أرجع succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    Cfg_بلوک* ثابت joinp = جمع_کن_بلوک();
    جمع_کن_گرفته_یال(joinp, succp);
    گرفته_Edgep->relink_بالاترین(joinp);
    untknEdgep->relink_بالاترین(joinp);
    أرجع joinp;
}

منطقي Cfg_گراف::درج_Two_Way_Joins() {
    // Reset marks
    لكل (V3_گراف_رأس& v : vertices()) v.user(0);

    منطقي isSeriesParallel = درست;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    لكل (V3_گراف_رأس& v : vertices()) {
        Cfg_بلوک& bb = static_cast<Cfg_بلوک&>(v);
        // Skip if already visited
        إذا (bb.user()) استمر;
        // Skip if not a branch
        إذا (!bb.isBranch()) استمر;
        // Fix it up, record if failed
        إذا (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = خطأ;
    }

    إذا (m_nEdits != m_n_آخرین_Ordered) rpo_بلوکها();
    إذا (dump_گراف_Level() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    أرجع isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

ثابت Cfg_بلوک* Cfg_Dominator_درخت::intersect(ثابت Cfg_بلوک* ap, ثابت Cfg_بلوک* bp) {
    طالما (ap != bp) {
        طالما (*ap > *bp) ap = m_bb2Idom[*ap];
        طالما (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    أرجع ap;
}

Cfg_Dominator_درخت::Cfg_Dominator_درخت(ثابت Cfg_گراف& cfg)
    : m_bb2Idom{cfg.make_بلوک_Map<ثابت Cfg_بلوک*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    لكل (منطقي تغییریافته = درست; تغییریافته;) {
        تغییریافته = خطأ;
        // For each vertex except enter block
        لكل (ثابت V3_گراف_رأس& vtx : cfg.vertices()) {
            ثابت Cfg_بلوک& curr = static_cast<ثابت Cfg_بلوک&>(vtx);
            إذا (curr.isEnter()) استمر;  // Skip entry block

            // For each predecessor of current block
            ثابت Cfg_بلوک* idom = اشاره_تهی;
            لكل (ثابت V3_گراف_یال& یال : curr.درون_یالها()) {
                ثابت Cfg_بلوک& pred = static_cast<ثابت Cfg_بلوک&>(*یال.fromp());
                // Skip if perdecessor not yet processed
                إذا (!m_bb2Idom[pred]) استمر;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            إذا (idom == m_bb2Idom[curr]) استمر;
            m_bb2Idom[curr] = idom;
            تغییریافته = درست;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = اشاره_تهی;
}
