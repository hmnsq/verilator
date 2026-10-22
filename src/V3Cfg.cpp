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

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Cfg.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitV.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// CfgBlock method definitions

std::string Cfg_ਖੰਡ::name() ਅਚਲ {
    std::stringstream ss;
    ss << "BB " + std::ਮੰਜ਼ਿਲ_string(id()) + ":\n";
    ਲਈ (Ast_ਗੰਢ* nodep : m_stmtps) {
        ਜੇ (ਅਚਲ AstIf* ਅਚਲ ifp = VN_CAST(nodep, If)) {
            ss << "if (";
            V3EmitV::debug_Verilog_For_ਰੁੱਖ(ifp->condp(), ss);
            ss << ") ...";
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstLoopTest* ਅਚਲ testp = VN_CAST(nodep, LoopTest)) {
            ss << "if  (!";
            V3EmitV::debug_Verilog_For_ਰੁੱਖ(testp->condp(), ss);
            ss << ") break;";
        } ਨਹੀਂਤਾਂ {
            V3EmitV::debug_Verilog_For_ਰੁੱਖ(nodep, ss);
        }
    }
    std::string ਪਾਠ = VString::replaceSubstr(
        VString::replaceSubstr(ss.str(), "\n", "\\l        "), "\"", "\\\"");
    ਜੇ (isEnter()) ਪਾਠ = "**ENTER**\n" + ਪਾਠ;
    ਜੇ (isExit()) ਪਾਠ = ਪਾਠ + "\n**EXIT**";
    ਵਾਪਸ ਪਾਠ;
}
std::string Cfg_ਖੰਡ::dotShape() ਅਚਲ { ਵਾਪਸ "rect"; }
std::string Cfg_ਖੰਡ::dotRank() ਅਚਲ {
    ਜੇ (isEnter()) ਵਾਪਸ "source";
    ਜੇ (isExit()) ਵਾਪਸ "sink";
    ਵਾਪਸ "";
}

//######################################################################
// CfgEdge method definitions

std::string Cfg_ਕਿਨਾਰਾ::dotLabel() ਅਚਲ {
    std::string label = "E" + std::ਮੰਜ਼ਿਲ_string(id());
    ਅਚਲ Cfg_ਕਿਨਾਰਾ* ਅਚਲ untknp = srcp()->untknEdgep();
    ਜੇ (ਇਹ == untknp) {
        label += " / F";
    } ਨਹੀਂਤਾਂ ਜੇ (untknp) {
        label += " / T";
    }
    ਵਾਪਸ label;
}

//######################################################################
// CfgGraph method definitions

ਸਥਿਰ ਖਾਲੀ cfg_ਕ੍ਰਮ_ਦੇਖੋ_ਖੰਡ(std::ਸਦਿਸ਼<Cfg_ਖੰਡ*>& post_ਕ੍ਰਮ_Enumeration, Cfg_ਖੰਡ* bbp) {
    // Mark visited
    bbp->user(1);
    // Visit un-visited successors
    ਜੇ (Cfg_ਖੰਡ* ਅਚਲ takenp = bbp->takenp()) {
        ਜੇ (!takenp->user()) cfg_ਕ੍ਰਮ_ਦੇਖੋ_ਖੰਡ(post_ਕ੍ਰਮ_Enumeration, takenp);
        ਜੇ (Cfg_ਖੰਡ* ਅਚਲ untknp = bbp->untknp()) {
            ਜੇ (!untknp->user()) cfg_ਕ੍ਰਮ_ਦੇਖੋ_ਖੰਡ(post_ਕ੍ਰਮ_Enumeration, untknp);
        }
    }
    // Add to post order enumeration
    post_ਕ੍ਰਮ_Enumeration.emplace_ਪਿੱਠ(bbp);
};

ਖਾਲੀ Cfg_ਗ੍ਰਾਫ::rpo_ਖੰਡਾਂ() {
    UASSERT_OBJ(m_nEdits != m_n_ਆਖਰੀ_Ordered, m_enterp, "Redundant 'CfgGraph::order' call");
    m_n_ਆਖਰੀ_Ordered = m_nEdits;

    // Reset marks
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& v : vertices()) v.user(0);

    // Compute post-order enumeration. Simple recursive algorith will do.
    std::ਸਦਿਸ਼<Cfg_ਖੰਡ*> post_ਕ੍ਰਮ_Enumeration;
    post_ਕ੍ਰਮ_Enumeration.reserve(m_n_ਖੰਡਾਂ);
    cfg_ਕ੍ਰਮ_ਦੇਖੋ_ਖੰਡ(post_ਕ੍ਰਮ_Enumeration, m_enterp);
    UASSERT_OBJ(post_ਕ੍ਰਮ_Enumeration.ਮਾਪ() == m_n_ਖੰਡਾਂ, m_enterp, "Inconsistent block count");

    // Assign block IDs equal to the reverse post-order number and sort vertices
    ਲਈ (ਮਾਪ_t i = 0; i < post_ਕ੍ਰਮ_Enumeration.ਮਾਪ(); ++i) {
        Cfg_ਖੰਡ* ਅਚਲ bbp = post_ਕ੍ਰਮ_Enumeration[m_n_ਖੰਡਾਂ - 1 - i];
        bbp->m_rpo_ਸੰਖਿਆ = i;
        vertices().unlink(bbp);
        vertices().link_ਪਿੱਠ(bbp);
    }

    // Assign edge IDs
    ਮਾਪ_t ਕਿਨਾਰਾ_ਗਿਣਤੀ = 0;
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& v : vertices()) {
        // cppcheck-suppress constVariableReference // cppcheck is wrong
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& e : v.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) static_cast<Cfg_ਕਿਨਾਰਾ&>(e).m_id = ਕਿਨਾਰਾ_ਗਿਣਤੀ++;
    }
    UASSERT_OBJ(ਕਿਨਾਰਾ_ਗਿਣਤੀ == m_n_ਕਿਨਾਰੇ, m_enterp, "Inconsistent edge count");
}

ਤਾਰਕਿਕ Cfg_ਗ੍ਰਾਫ::ਸ਼ਾਮਲ_ਹੈ_Loop() ਅਚਲ {
    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : vertices()) {
        ਅਚਲ Cfg_ਖੰਡ& current = static_cast<ਅਚਲ Cfg_ਖੰਡ&>(vtx);
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : current.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            ਅਚਲ Cfg_ਖੰਡ& successor = *static_cast<ਅਚਲ Cfg_ਖੰਡ*>(ਕਿਨਾਰਾ.ਸਿਖਰ());
            // IDs are the reverse post-order numbering, so easy to check for a back-edge
            ਜੇ (successor.id() < current.id()) ਵਾਪਸ ਸੱਚ;
        }
    }
    ਵਾਪਸ ਝੂਠ;
}

ਖਾਲੀ Cfg_ਗ੍ਰਾਫ::minimize() {
    // Remove empty blocks (except enter and exit)
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : vertices().unlinkable()) {
        Cfg_ਖੰਡ* ਅਚਲ bbp = static_cast<Cfg_ਖੰਡ*>(vtxp);
        ਜੇ (bbp->isEnter()) ਜਾਰੀ;
        ਜੇ (bbp->isExit()) ਜਾਰੀ;
        ਜੇ (!bbp->stmtps().ਖਾਲੀ_ਥਾਵਾਂ()) ਜਾਰੀ;
        UASSERT(!bbp->isBranch(), "Empty block should have a single successor");
        Cfg_ਖੰਡ* ਅਚਲ succp = bbp->takenp();
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep : bbp->ਵਿੱਚ_ਕਿਨਾਰੇ().unlinkable()) edgep->relink_ਸਿਖਰ(succp);
        ++m_nEdits;
        --m_n_ਕਿਨਾਰੇ;
        --m_n_ਖੰਡਾਂ;
        VL_DO_DANGLING(bbp->unlinkDelete(ਇਹ), bbp);
    }

    // Combine sequential blocks
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : vertices().unlinkable()) {
        Cfg_ਖੰਡ* ਅਚਲ srcp = static_cast<Cfg_ਖੰਡ*>(vtxp);
        ਜੇ (srcp->isExit()) ਜਾਰੀ;
        ਜੇ (srcp->isBranch()) ਜਾਰੀ;
        Cfg_ਖੰਡ* ਅਚਲ dstp = srcp->takenp();
        ਜੇ (dstp->isJoin()) ਜਾਰੀ;
        // Combine them
        ਜੇ (srcp->isEnter()) m_enterp = dstp;
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_Stmt*> stmtps{std::move(srcp->m_stmtps)};
        stmtps.reserve(stmtps.ਮਾਪ() + dstp->m_stmtps.ਮਾਪ());
        stmtps.ਪਾਓ(stmtps.ਸਿਰਾ(), dstp->m_stmtps.begin(), dstp->m_stmtps.ਸਿਰਾ());
        dstp->m_stmtps = std::move(stmtps);
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep : srcp->ਵਿੱਚ_ਕਿਨਾਰੇ().unlinkable()) edgep->relink_ਸਿਖਰ(dstp);
        ++m_nEdits;
        --m_n_ਕਿਨਾਰੇ;
        --m_n_ਖੰਡਾਂ;
        VL_DO_DANGLING(srcp->unlinkDelete(ਇਹ), srcp);
    }

    ਜੇ (m_nEdits != m_n_ਆਖਰੀ_Ordered) rpo_ਖੰਡਾਂ();
    ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 9) dumpDotFilePrefixed("cfg-minimize");
}

ਖਾਲੀ Cfg_ਗ੍ਰਾਫ::break_Critical_ਕਿਨਾਰੇ() {
    // Gather critical edges
    std::ਸਦਿਸ਼<Cfg_ਕਿਨਾਰਾ*> critical_ਕਿਨਾਰੇ;
    critical_ਕਿਨਾਰੇ.reserve(m_n_ਕਿਨਾਰੇ);
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : vertices()) {
        ਅਚਲ Cfg_ਖੰਡ& bb = static_cast<ਅਚਲ Cfg_ਖੰਡ&>(vtx);
        ਜੇ (!bb.isBranch()) ਜਾਰੀ;
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtx.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            ਅਚਲ Cfg_ਖੰਡ& succ = static_cast<ਅਚਲ Cfg_ਖੰਡ&>(*ਕਿਨਾਰਾ.ਸਿਖਰ());
            ਜੇ (!succ.isJoin()) ਜਾਰੀ;
            critical_ਕਿਨਾਰੇ.emplace_ਪਿੱਠ(static_cast<Cfg_ਕਿਨਾਰਾ*>(&ਕਿਨਾਰਾ));
        }
    }
    // Insert blocks
    ਲਈ (Cfg_ਕਿਨਾਰਾ* ਅਚਲ edgep : critical_ਕਿਨਾਰੇ) {
        Cfg_ਖੰਡ* ਅਚਲ newp = ਜੋੜੋ_ਖੰਡ();
        ਜੋੜੋ_ਲਿਆ_ਕਿਨਾਰਾ(newp, edgep->dstp());
        edgep->relink_ਸਿਖਰ(newp);
    }

    ਜੇ (m_nEdits != m_n_ਆਖਰੀ_Ordered) rpo_ਖੰਡਾਂ();
    ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 9) dumpDotFilePrefixed("cfg-breakCriticalEdges");
}

// Given a branching basic block, if the sub-graph below this branch, up until
// the point where all of its control flow path convertes is series-parallel,
// then return the (potentially newly created) basic block with exactly 2
// predecessors where the two control flow paths from this branch have joined.
// If the relevant sub-graph is not series-parallel (there is a control flow
// path between the branches, or to a path not dominated by the given branch),
// then return nullptr. Cached results in the given map
Cfg_ਖੰਡ* Cfg_ਗ੍ਰਾਫ::getOrCreateTwoWayJoinFor(Cfg_ਖੰਡ* bbp) {
    UASSERT_OBJ(bbp->isBranch(), bbp, "Not a branch");

    // Mark visited
    UASSERT_OBJ(!bbp->user(), bbp, "Should not visit twice");
    bbp->user(1);

    // We need the edge converting to a join block along both path. This is how we find it:
    ਅਚਲ ਸਵੈਚਾਲਿਤ chase_ਕਿਨਾਰਾ = [&](Cfg_ਕਿਨਾਰਾ* edgep) -> Cfg_ਕਿਨਾਰਾ* {
        ਜਦੋਂ (ਸੱਚ) {
            Cfg_ਖੰਡ* dstp = edgep->dstp();
            // Stop if found the joining block along this path
            ਜੇ (dstp->isJoin()) ਵਾਪਸ edgep;
            // If the successor is a branch, recursively get it's 2-way join block
            ਜਦੋਂ (dstp->isBranch()) {
                dstp = getOrCreateTwoWayJoinFor(dstp);
                // If the subgarph below dstp is not series-parallel, then no solution
                ਜੇ (!dstp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            }
            UASSERT_OBJ(!dstp->isExit(), bbp, "Non-convergent branch - multiple Exit blocks?");
            edgep = dstp->ਲਿਆ_Edgep();
        }
    };

    // Walk down both paths
    Cfg_ਕਿਨਾਰਾ* ਅਚਲ ਲਿਆ_Edgep = chase_ਕਿਨਾਰਾ(bbp->ਲਿਆ_Edgep());
    ਜੇ (!ਲਿਆ_Edgep) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    Cfg_ਕਿਨਾਰਾ* ਅਚਲ untknEdgep = chase_ਕਿਨਾਰਾ(bbp->untknEdgep());
    ਜੇ (!untknEdgep) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    // If we ended up at different joining blocks, then there is a path from one
    // of the branches into a path of another branch before 'bbp', no solution
    ਜੇ (ਲਿਆ_Edgep->dstp() != untknEdgep->dstp()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

    // Pick up the common successor
    Cfg_ਖੰਡ* ਅਚਲ succp = ਲਿਆ_Edgep->dstp();
    // If the common successor is a 2-way join, we can use it directly
    ਜੇ (succp->isTwoWayJoin()) ਵਾਪਸ succp;
    // Otherwise insert a new block to join the 2 paths of the original block
    Cfg_ਖੰਡ* ਅਚਲ joinp = ਜੋੜੋ_ਖੰਡ();
    ਜੋੜੋ_ਲਿਆ_ਕਿਨਾਰਾ(joinp, succp);
    ਲਿਆ_Edgep->relink_ਸਿਖਰ(joinp);
    untknEdgep->relink_ਸਿਖਰ(joinp);
    ਵਾਪਸ joinp;
}

ਤਾਰਕਿਕ Cfg_ਗ੍ਰਾਫ::ਪਾਓ_Two_Way_Joins() {
    // Reset marks
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& v : vertices()) v.user(0);

    ਤਾਰਕਿਕ isSeriesParallel = ਸੱਚ;

    // We will be adding vertices at the end. That's OK, they don't need to be visited again
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& v : vertices()) {
        Cfg_ਖੰਡ& bb = static_cast<Cfg_ਖੰਡ&>(v);
        // Skip if already visited
        ਜੇ (bb.user()) ਜਾਰੀ;
        // Skip if not a branch
        ਜੇ (!bb.isBranch()) ਜਾਰੀ;
        // Fix it up, record if failed
        ਜੇ (!getOrCreateTwoWayJoinFor(&bb)) isSeriesParallel = ਝੂਠ;
    }

    ਜੇ (m_nEdits != m_n_ਆਖਰੀ_Ordered) rpo_ਖੰਡਾਂ();
    ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 9) dumpDotFilePrefixed("cfg-insertTwoWayJoins");

    ਵਾਪਸ isSeriesParallel;
}

//######################################################################
// CfgDominatorTree

ਅਚਲ Cfg_ਖੰਡ* Cfg_Dominator_ਰੁੱਖ::intersect(ਅਚਲ Cfg_ਖੰਡ* ap, ਅਚਲ Cfg_ਖੰਡ* bp) {
    ਜਦੋਂ (ap != bp) {
        ਜਦੋਂ (*ap > *bp) ap = m_bb2Idom[*ap];
        ਜਦੋਂ (*bp > *ap) bp = m_bb2Idom[*bp];
    }
    ਵਾਪਸ ap;
}

Cfg_Dominator_ਰੁੱਖ::Cfg_Dominator_ਰੁੱਖ(ਅਚਲ Cfg_ਗ੍ਰਾਫ& cfg)
    : m_bb2Idom{cfg.make_ਖੰਡ_Map<ਅਚਲ Cfg_ਖੰਡ*>()} {

    // Build the immediate dominator map, using algorithm from:
    // "A Simple, Fast Dominance Algorithm", Keith D. Cooper et al., 2006
    // Immediate dominator of the enter block

    // Point enteer block to itself, while computing below
    m_bb2Idom[cfg.enter()] = &cfg.enter();
    // Iterate until settled
    ਲਈ (ਤਾਰਕਿਕ ਬਦਲਿਆ = ਸੱਚ; ਬਦਲਿਆ;) {
        ਬਦਲਿਆ = ਝੂਠ;
        // For each vertex except enter block
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : cfg.vertices()) {
            ਅਚਲ Cfg_ਖੰਡ& curr = static_cast<ਅਚਲ Cfg_ਖੰਡ&>(vtx);
            ਜੇ (curr.isEnter()) ਜਾਰੀ;  // Skip entry block

            // For each predecessor of current block
            ਅਚਲ Cfg_ਖੰਡ* idom = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : curr.ਵਿੱਚ_ਕਿਨਾਰੇ()) {
                ਅਚਲ Cfg_ਖੰਡ& pred = static_cast<ਅਚਲ Cfg_ਖੰਡ&>(*ਕਿਨਾਰਾ.fromp());
                // Skip if perdecessor not yet processed
                ਜੇ (!m_bb2Idom[pred]) ਜਾਰੀ;
                // Pick first, then use intersect
                idom = !idom ? &pred : intersect(&pred, idom);
            }

            // If chenged, record it, else move on
            ਜੇ (idom == m_bb2Idom[curr]) ਜਾਰੀ;
            m_bb2Idom[curr] = idom;
            ਬਦਲਿਆ = ਸੱਚ;
        }
    }

    // The enter block is the root of the tree and does not itself have an immediate dominator
    m_bb2Idom[cfg.enter()] = ਖਾਲੀ_ਸੂਚਕ;
}
