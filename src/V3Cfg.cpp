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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Cfg.h"

#شامل "V3Ast.h"
#شامل "V3EmitV.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// CfgBlock method definitions

std::string Cfg_بلاک::name() ثابت {
    std::stringstream ss;
    ss << "BB " + std::مقصد_string(id()) + ":\n";
    برائے (Ast_گرہ* nodep : m_stmtps) {
        اگر (ثابت AstIf* ثابت ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debug_Verilog_For_درخت(ifp->condp(), ss);
            ss << ") ...";
        } ورنہ اگر (ثابت AstLoopTest* ثابت testp = VN_CAST(nodep, LoopTest)) {
            ss << "if  (!";
            V3EmitV::debug_Verilog_For_درخت(testp->condp(), ss);
            ss << ") break;";
        } ورنہ {
            V3EmitV::debug_Verilog_For_درخت(nodep, ss);
        }
    }
    std::string متن = VString::replaceSubstr(
        VString::replaceSubstr(ss.str(), "\n", "\\l        "), "\"", "\\\"");
    اگر (isEnter()) متن = "**ENTER**\n" + متن;
    اگر (isExit()) متن = متن + "\n**EXIT**";
    واپس متن;
}
std::string Cfg_بلاک::dotShape() ثابت { واپس "rect"; }
std::string Cfg_بلاک::dotRank() ثابت {
    اگر (isEnter()) واپس "source";
    اگر (isExit()) واپس "sink";
    واپس "";
}

//######################################################################
// CfgEdge method definitions

std::string Cfg_کنارہ::dotLabel() ثابت {
    std::string label = "E" + std::مقصد_string(id());
    ثابت Cfg_کنارہ* ثابت untknp = srcp()->untknEdgep();
    اگر (یہ == untknp) {
        label += " / F";
    } ورنہ اگر (untknp) {
        label += " / T";
    }
    واپس label;
}

//######################################################################
// CfgGraph method definitions

ساکن خالی cfg_ترتیب_دورہ_بلاک(std::سمتیہ<Cfg_بلاک*>& post_ترتیب_Enumeration, Cfg_بلاک* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    اگر (Cfg_بلاک* ثابت takenp = bbp->takenp()) {
        اگر (!takenp->user()) cfg_ترتیب_دورہ_بلاک(post_ترتیب_Enumeration, takenp);
        اگر (Cfg_بلاک* ثابت untknp = bbp->untknp()) {
            اگر (!untknp->user()) cfg_ترتیب_دورہ_بلاک(post_ترتیب_Enumeration, untknp);
        }
    }
    // Add to post order enumeration
    post_ترتیب_Enumeration.emplace_عقب(bbp);
};

خالی Cfg_گراف::rpo_بلاکیں() {
    UASSERT_OBJ(m_nEdits != m_n_آخری_Ordered, m_enterp, "Redundant 'CfgGraph::order' call");
    m_n_آخری_Ordered = m_nEdits;

    // Reset marks
    برائے (V3_گراف_راس& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::سمتیہ<Cfg_بلاک*> post_ترتیب_Enumeration;
    post_ترتیب_Enumeration.reserve(m_n_بلاکیں);
    cfg_ترتیب_دورہ_بلاک(post_ترتیب_Enumeration, m_enterp);
    UASSERT_OBJ(post_ترتیب_Enumeration.سائز() == m_n_بلاکیں, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    برائے (سائز_t i = 0; i < post_ترتیب_Enumeration.سائز(); ++i) {
        Cfg_بلاک* ثابت bbp = post_ترتیب_Enumeration[m_n_بلاکیں - 1 - i];
        bbp->m_rpo_نمبر = i;
        vertices().unlink(bbp);
        vertices().link_عقب(bbp);
    }

    // Assign edge IDs
    سائز_t کنارہ_گنتی = 0;
    برائے (V3_گراف_راس& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        برائے (V3_گراف_کنارہ& e : v.برآمد_کنارے()) static_cast<Cfg_کنارہ&>(e).m_id = کنارہ_گنتی++;
    }
    UASSERT_OBJ(کنارہ_گنتی == m_n_کنارے, m_enterp, "Inconsistent edge count");
}

منطقی Cfg_گراف::شامل_ہے_Loop() ثابت {
    برائے (ثابت V3_گراف_راس& vtx : vertices()) {
        ثابت Cfg_بلاک& current = static_cast<ثابت Cfg_بلاک&>(vtx);
        برائے (ثابت V3_گراف_کنارہ& کنارہ : current.برآمد_کنارے()) {
            ثابت Cfg_بلاک& successor = *static_cast<ثابت Cfg_بلاک*>(کنارہ.چوٹی());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            اگر (successor.id() < current.id()) واپس سچ;
        }
    }
    واپس جھوٹ;
}

خالی Cfg_گراف::minimize() {
    // Remove empty blocks (except enter and exit)
    برائے (V3_گراف_راس* ثابت vtxp : vertices().unlinkable()) {
        Cfg_بلاک* ثابت bbp = static_cast<Cfg_بلاک*>(vtxp);
        اگر (bbp->isEnter()) جاری;
        اگر (bbp->isExit()) جاری;
        اگر (!bbp->stmtps().خالی_خانے()) جاری;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        Cfg_بلاک* ثابت succp = bbp->takenp();
        برائے (V3_گراف_کنارہ* ثابت edgep : bbp->اندر_کنارے().unlinkable()) edgep->relink_چوٹی(succp);
        ++m_nEdits;
        --m_n_کنارے;
        --m_n_بلاکیں;
        VL_DO_DANGLING(bbp->unlinkDelete(یہ), bbp);
    }

    // Combine sequential blocks
    برائے (V3_گراف_راس* ثابت vtxp : vertices().unlinkable()) {
        Cfg_بلاک* ثابت srcp = static_cast<Cfg_بلاک*>(vtxp);
        اگر (srcp->isExit()) جاری;
        اگر (srcp->isBranch()) جاری;
        Cfg_بلاک* ثابت dstp = srcp->takenp();
        اگر (dstp->isJoin()) جاری;
        // Combine them
        اگر (srcp->isEnter()) m_enterp = dstp;
        std::سمتیہ<Ast_گرہ_Stmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.سائز() + dstp->m_stmtps.سائز());
        stmtps.داخل_کرو(stmtps.انتہا(), dstp->m_stmtps.begin(), dstp->m_stmtps.انتہا());
        dstp->m_stmtps = std::move(stmtps);
        برائے (V3_گراف_کنارہ* ثابت edgep : srcp->اندر_کنارے().unlinkable()) edgep->relink_چوٹی(dstp);
        ++m_nEdits;
        --m_n_کنارے;
        --m_n_بلاکیں;
        VL_DO_DANGLING(srcp->unlinkDelete(یہ), srcp);
    }

    اگر (m_nEdits != m_n_آخری_Ordered) rpo_بلاکیں();
    اگر (dump_گراف_Level() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

خالی Cfg_گراف::break_Critical_کنارے() {
    // Gather critical edges
    std::سمتیہ<Cfg_کنارہ*> critical_کنارے;
    critical_کنارے.reserve(m_n_کنارے);
    برائے (V3_گراف_راس& vtx : vertices()) {
        ثابت Cfg_بلاک& bb = static_cast<ثابت Cfg_بلاک&>(vtx);
        اگر (!bb.isBranch()) جاری;
        برائے (V3_گراف_کنارہ& کنارہ : vtx.برآمد_کنارے()) {
            ثابت Cfg_بلاک& succ = static_cast<ثابت Cfg_بلاک&>(*کنارہ.چوٹی());
            اگر (!succ.isJoin()) جاری;
            critical_کنارے.emplace_عقب(static_cast<Cfg_کنارہ*>(&کنارہ));
        }
    }
    // Insert blocks
    برائے (Cfg_کنارہ* ثابت edgep : critical_کنارے) {
        Cfg_بلاک* ثابت newp = جوڑو_بلاک();
        جوڑو_لیا_گیا_کنارہ(newp, edgep->dstp());
        edgep->relink_چوٹی(newp);
    }

    اگر (m_nEdits != m_n_آخری_Ordered) rpo_بلاکیں();
    اگر (dump_گراف_Level() >= 9) dumpDotFilePrefixed("cfg-breakCriticalEdges");
}

// Given a branching basic block, if the sub-graph below this branch, up until
// the point where all of its control flow path convertes is series-parallel,
// then return the (potentially newly created) basic block with exactly 2
// predecessors where the two control flow paths from this branch have joined.
// If the relevant sub-graph is not series-parallel (there is a control flow
// path between the branches, or to a path not dominated by the given branch),
// then return nullptr. Cached results in the given map
Cfg_بلاک* Cfg_گراف::getOrCreateTwoWayJoinFor(Cfg_بلاک* bbp) {
    UASSERT_OBJ(bbp->isBranch(), bbp, "Not a branch");

    // Mark visited
    UASSERT_OBJ(!bbp->user(), bbp, "Should not visit twice");
    bbp->user(1);

    // We need the edge converting to a join block along both path. This is how we find it:
    ثابت خودکار chase_کنارہ = [&](Cfg_کنارہ* edgep) -> Cfg_کنارہ* {
        جب (سچ) {
            Cfg_بلاک* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            اگر (dstp->isJoin()) واپس edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            جب (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                اگر (!dstp) واپس خالی_اشاریہ;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->لیا_گیا_Edgep();
        }
    };

    // Walk down both paths
    Cfg_کنارہ* ثابت لیا_گیا_Edgep = chase_کنارہ(bbp->لیا_گیا_Edgep());
    اگر (!لیا_گیا_Edgep) واپس خالی_اشاریہ;
    Cfg_کنارہ* ثابت untknEdgep = chase_کنارہ(bbp->untknEdgep());
    اگر (!untknEdgep) واپس خالی_اشاریہ;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    اگر (لیا_گیا_Edgep->dstp() != untknEdgep->dstp()) واپس خالی_اشاریہ;

    // Pick up the common successor
    Cfg_بلاک* ثابت succp = لیا_گیا_Edgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    اگر (succp->isTwoWayJoin()) واپس succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    Cfg_بلاک* ثابت joinp = جوڑو_بلاک();
    جوڑو_لیا_گیا_کنارہ(joinp, succp);
    لیا_گیا_Edgep->relink_چوٹی(joinp);
    untknEdgep->relink_چوٹی(joinp);
    واپس joinp;
}

منطقی Cfg_گراف::داخل_کرو_Two_Way_Joins() {
    // Reset marks
    برائے (V3_گراف_راس& v : vertices()) v.user(0);

    منطقی isSeriesParallel = سچ;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    برائے (V3_گراف_راس& v : vertices()) {
        Cfg_بلاک& bb = static_cast<Cfg_بلاک&>(v);
        // Skip if already visited
        اگر (bb.user()) جاری;
        // Skip if not a branch
        اگر (!bb.isBranch()) جاری;
        // Fix it up, record if failed
        اگر (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = جھوٹ;
    }

    اگر (m_nEdits != m_n_آخری_Ordered) rpo_بلاکیں();
    اگر (dump_گراف_Level() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    واپس isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

ثابت Cfg_بلاک* Cfg_Dominator_درخت::intersect(ثابت Cfg_بلاک* ap, ثابت Cfg_بلاک* bp) {
    جب (ap != bp) {
        جب (*ap > *bp) ap = m_bb2Idom[*ap];
        جب (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    واپس ap;
}

Cfg_Dominator_درخت::Cfg_Dominator_درخت(ثابت Cfg_گراف& cfg)
    : m_bb2Idom{cfg.make_بلاک_Map<ثابت Cfg_بلاک*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    برائے (منطقی بدلا = سچ; بدلا;) {
        بدلا = جھوٹ;
        // For each vertex except enter block
        برائے (ثابت V3_گراف_راس& vtx : cfg.vertices()) {
            ثابت Cfg_بلاک& curr = static_cast<ثابت Cfg_بلاک&>(vtx);
            اگر (curr.isEnter()) جاری;  // Skip entry block

            // For each predecessor of current block
            ثابت Cfg_بلاک* idom = خالی_اشاریہ;
            برائے (ثابت V3_گراف_کنارہ& کنارہ : curr.اندر_کنارے()) {
                ثابت Cfg_بلاک& pred = static_cast<ثابت Cfg_بلاک&>(*کنارہ.fromp());
                // Skip if perdecessor not yet processed
                اگر (!m_bb2Idom[pred]) جاری;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            اگر (idom == m_bb2Idom[curr]) جاری;
            m_bb2Idom[curr] = idom;
            بدلا = سچ;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = خالی_اشاریہ;
}
