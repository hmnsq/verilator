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

std::string Cfg_بلاک::name() ثابت {
    std::stringstream ss;
    ss << "BB " + std::منزل_string(id()) + ":\n";
    لكل (Ast_غوټه* nodep : m_stmtps) {
        إذا (ثابت AstIf* ثابت ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debug_Verilog_For_ونه(ifp->condp(), ss);
            ss << ") ...";
        } وإلا إذا (ثابت AstLoopTest* ثابت testp = VN_CAST(nodep, LoopTest)) {
            ss << "if  (!";
            V3EmitV::debug_Verilog_For_ونه(testp->condp(), ss);
            ss << ") break;";
        } وإلا {
            V3EmitV::debug_Verilog_For_ونه(nodep, ss);
        }
    }
    std::string متن = VString::replaceSubstr(
        VString::replaceSubstr(ss.str(), "\n", "\\l        "), "\"", "\\\"");
    إذا (isEnter()) متن = "**ENTER**\n" + متن;
    إذا (isExit()) متن = متن + "\n**EXIT**";
    أرجع متن;
}
std::string Cfg_بلاک::dotShape() ثابت { أرجع "rect"; }
std::string Cfg_بلاک::dotRank() ثابت {
    إذا (isEnter()) أرجع "source";
    إذا (isExit()) أرجع "sink";
    أرجع "";
}

//######################################################################
// CfgEdge method definitions

std::string Cfg_څنډه::dotLabel() ثابت {
    std::string label = "E" + std::منزل_string(id());
    ثابت Cfg_څنډه* ثابت untknp = srcp()->untknEdgep();
    إذا (هذا == untknp) {
        label += " / F";
    } وإلا إذا (untknp) {
        label += " / T";
    }
    أرجع label;
}

//######################################################################
// CfgGraph method definitions

ساكن فراغ cfg_ترتیب_لیدنه_بلاک(std::ویکټور<Cfg_بلاک*>& post_ترتیب_Enumeration, Cfg_بلاک* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    إذا (Cfg_بلاک* ثابت takenp = bbp->takenp()) {
        إذا (!takenp->user()) cfg_ترتیب_لیدنه_بلاک(post_ترتیب_Enumeration, takenp);
        إذا (Cfg_بلاک* ثابت untknp = bbp->untknp()) {
            إذا (!untknp->user()) cfg_ترتیب_لیدنه_بلاک(post_ترتیب_Enumeration, untknp);
        }
    }
    // Add to post order enumeration
    post_ترتیب_Enumeration.emplace_شا(bbp);
};

فراغ Cfg_ګراف::rpo_بلاکونه() {
    UASSERT_OBJ(m_nEdits != m_n_وروستی_Ordered, m_enterp, "Redundant 'CfgGraph::order' call");
    m_n_وروستی_Ordered = m_nEdits;

    // Reset marks
    لكل (V3_ګراف_راس& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::ویکټور<Cfg_بلاک*> post_ترتیب_Enumeration;
    post_ترتیب_Enumeration.reserve(m_n_بلاکونه);
    cfg_ترتیب_لیدنه_بلاک(post_ترتیب_Enumeration, m_enterp);
    UASSERT_OBJ(post_ترتیب_Enumeration.اندازه() == m_n_بلاکونه, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    لكل (اندازه_t i = 0; i < post_ترتیب_Enumeration.اندازه(); ++i) {
        Cfg_بلاک* ثابت bbp = post_ترتیب_Enumeration[m_n_بلاکونه - 1 - i];
        bbp->m_rpo_شمېره = i;
        vertices().unlink(bbp);
        vertices().link_شا(bbp);
    }

    // Assign edge IDs
    اندازه_t څنډه_شمېر = 0;
    لكل (V3_ګراف_راس& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        لكل (V3_ګراف_څنډه& e : v.بهر_څنډې()) static_cast<Cfg_څنډه&>(e).m_id = څنډه_شمېر++;
    }
    UASSERT_OBJ(څنډه_شمېر == m_n_څنډې, m_enterp, "Inconsistent edge count");
}

منطقي Cfg_ګراف::لري_Loop() ثابت {
    لكل (ثابت V3_ګراف_راس& vtx : vertices()) {
        ثابت Cfg_بلاک& current = static_cast<ثابت Cfg_بلاک&>(vtx);
        لكل (ثابت V3_ګراف_څنډه& څنډه : current.بهر_څنډې()) {
            ثابت Cfg_بلاک& successor = *static_cast<ثابت Cfg_بلاک*>(څنډه.سر());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            إذا (successor.id() < current.id()) أرجع ریښتیا;
        }
    }
    أرجع خطأ;
}

فراغ Cfg_ګراف::minimize() {
    // Remove empty blocks (except enter and exit)
    لكل (V3_ګراف_راس* ثابت vtxp : vertices().unlinkable()) {
        Cfg_بلاک* ثابت bbp = static_cast<Cfg_بلاک*>(vtxp);
        إذا (bbp->isEnter()) استمر;
        إذا (bbp->isExit()) استمر;
        إذا (!bbp->stmtps().تشه()) استمر;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        Cfg_بلاک* ثابت succp = bbp->takenp();
        لكل (V3_ګراف_څنډه* ثابت edgep : bbp->دننه_څنډې().unlinkable()) edgep->relink_سر(succp);
        ++m_nEdits;
        --m_n_څنډې;
        --m_n_بلاکونه;
        VL_DO_DANGLING(bbp->unlinkDelete(هذا), bbp);
    }

    // Combine sequential blocks
    لكل (V3_ګراف_راس* ثابت vtxp : vertices().unlinkable()) {
        Cfg_بلاک* ثابت srcp = static_cast<Cfg_بلاک*>(vtxp);
        إذا (srcp->isExit()) استمر;
        إذا (srcp->isBranch()) استمر;
        Cfg_بلاک* ثابت dstp = srcp->takenp();
        إذا (dstp->isJoin()) استمر;
        // Combine them
        إذا (srcp->isEnter()) m_enterp = dstp;
        std::ویکټور<Ast_غوټه_Stmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.اندازه() + dstp->m_stmtps.اندازه());
        stmtps.ورننباسه(stmtps.پای_ټکی(), dstp->m_stmtps.begin(), dstp->m_stmtps.پای_ټکی());
        dstp->m_stmtps = std::move(stmtps);
        لكل (V3_ګراف_څنډه* ثابت edgep : srcp->دننه_څنډې().unlinkable()) edgep->relink_سر(dstp);
        ++m_nEdits;
        --m_n_څنډې;
        --m_n_بلاکونه;
        VL_DO_DANGLING(srcp->unlinkDelete(هذا), srcp);
    }

    إذا (m_nEdits != m_n_وروستی_Ordered) rpo_بلاکونه();
    إذا (dump_ګراف_Level() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

فراغ Cfg_ګراف::break_Critical_څنډې() {
    // Gather critical edges
    std::ویکټور<Cfg_څنډه*> critical_څنډې;
    critical_څنډې.reserve(m_n_څنډې);
    لكل (V3_ګراف_راس& vtx : vertices()) {
        ثابت Cfg_بلاک& bb = static_cast<ثابت Cfg_بلاک&>(vtx);
        إذا (!bb.isBranch()) استمر;
        لكل (V3_ګراف_څنډه& څنډه : vtx.بهر_څنډې()) {
            ثابت Cfg_بلاک& succ = static_cast<ثابت Cfg_بلاک&>(*څنډه.سر());
            إذا (!succ.isJoin()) استمر;
            critical_څنډې.emplace_شا(static_cast<Cfg_څنډه*>(&څنډه));
        }
    }
    // Insert blocks
    لكل (Cfg_څنډه* ثابت edgep : critical_څنډې) {
        Cfg_بلاک* ثابت newp = جمع_کړه_بلاک();
        جمع_کړه_اخیستل_شوی_څنډه(newp, edgep->dstp());
        edgep->relink_سر(newp);
    }

    إذا (m_nEdits != m_n_وروستی_Ordered) rpo_بلاکونه();
    إذا (dump_ګراف_Level() >= 9) dumpDotFilePrefixed("cfg-breakCriticalEdges");
}

// Given a branching basic block, if the sub-graph below this branch, up until
// the point where all of its control flow path convertes is series-parallel,
// then return the (potentially newly created) basic block with exactly 2
// predecessors where the two control flow paths from this branch have joined.
// If the relevant sub-graph is not series-parallel (there is a control flow
// path between the branches, or to a path not dominated by the given branch),
// then return nullptr. Cached results in the given map
Cfg_بلاک* Cfg_ګراف::getOrCreateTwoWayJoinFor(Cfg_بلاک* bbp) {
    UASSERT_OBJ(bbp->isBranch(), bbp, "Not a branch");

    // Mark visited
    UASSERT_OBJ(!bbp->user(), bbp, "Should not visit twice");
    bbp->user(1);

    // We need the edge converting to a join block along both path. This is how we find it:
    ثابت خپلکاری chase_څنډه = [&](Cfg_څنډه* edgep) -> Cfg_څنډه* {
        طالما (ریښتیا) {
            Cfg_بلاک* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            إذا (dstp->isJoin()) أرجع edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            طالما (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                إذا (!dstp) أرجع تش_اشاره;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->اخیستل_شوی_Edgep();
        }
    };

    // Walk down both paths
    Cfg_څنډه* ثابت اخیستل_شوی_Edgep = chase_څنډه(bbp->اخیستل_شوی_Edgep());
    إذا (!اخیستل_شوی_Edgep) أرجع تش_اشاره;
    Cfg_څنډه* ثابت untknEdgep = chase_څنډه(bbp->untknEdgep());
    إذا (!untknEdgep) أرجع تش_اشاره;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    إذا (اخیستل_شوی_Edgep->dstp() != untknEdgep->dstp()) أرجع تش_اشاره;

    // Pick up the common successor
    Cfg_بلاک* ثابت succp = اخیستل_شوی_Edgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    إذا (succp->isTwoWayJoin()) أرجع succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    Cfg_بلاک* ثابت joinp = جمع_کړه_بلاک();
    جمع_کړه_اخیستل_شوی_څنډه(joinp, succp);
    اخیستل_شوی_Edgep->relink_سر(joinp);
    untknEdgep->relink_سر(joinp);
    أرجع joinp;
}

منطقي Cfg_ګراف::ورننباسه_Two_Way_Joins() {
    // Reset marks
    لكل (V3_ګراف_راس& v : vertices()) v.user(0);

    منطقي isSeriesParallel = ریښتیا;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    لكل (V3_ګراف_راس& v : vertices()) {
        Cfg_بلاک& bb = static_cast<Cfg_بلاک&>(v);
        // Skip if already visited
        إذا (bb.user()) استمر;
        // Skip if not a branch
        إذا (!bb.isBranch()) استمر;
        // Fix it up, record if failed
        إذا (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = خطأ;
    }

    إذا (m_nEdits != m_n_وروستی_Ordered) rpo_بلاکونه();
    إذا (dump_ګراف_Level() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    أرجع isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

ثابت Cfg_بلاک* Cfg_Dominator_ونه::intersect(ثابت Cfg_بلاک* ap, ثابت Cfg_بلاک* bp) {
    طالما (ap != bp) {
        طالما (*ap > *bp) ap = m_bb2Idom[*ap];
        طالما (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    أرجع ap;
}

Cfg_Dominator_ونه::Cfg_Dominator_ونه(ثابت Cfg_ګراف& cfg)
    : m_bb2Idom{cfg.make_بلاک_Map<ثابت Cfg_بلاک*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    لكل (منطقي بدل_شوی = ریښتیا; بدل_شوی;) {
        بدل_شوی = خطأ;
        // For each vertex except enter block
        لكل (ثابت V3_ګراف_راس& vtx : cfg.vertices()) {
            ثابت Cfg_بلاک& curr = static_cast<ثابت Cfg_بلاک&>(vtx);
            إذا (curr.isEnter()) استمر;  // Skip entry block

            // For each predecessor of current block
            ثابت Cfg_بلاک* idom = تش_اشاره;
            لكل (ثابت V3_ګراف_څنډه& څنډه : curr.دننه_څنډې()) {
                ثابت Cfg_بلاک& pred = static_cast<ثابت Cfg_بلاک&>(*څنډه.fromp());
                // Skip if perdecessor not yet processed
                إذا (!m_bb2Idom[pred]) استمر;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            إذا (idom == m_bb2Idom[curr]) استمر;
            m_bb2Idom[curr] = idom;
            بدل_شوی = ریښتیا;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = تش_اشاره;
}
