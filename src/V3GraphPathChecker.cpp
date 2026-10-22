// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: DAG Path Checking
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

#define VL_MT_DISABLED_CODE_UNIT 1

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3GraphPathChecker.h"

#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3GraphStream.h"

//######################################################################
// GraphPCNode

સંરચના GraphPCNode final {
    // User data for each node in GraphPathChecker.
    //
    // Like the LogicMTasks's, store before and after CPs for the nodes in
    // the GraphPathChecker graph.
    //
    // Unlike the LogicMTasks's, we have no cost info for the generic graph
    // accepted by GraphPathChecker, so assume each node has unit cost.
    std::array<uint32_t, GraphWay::અંક_WAYS> m_cp = {};

    // Detect if we've seen this node before in a given recursive
    // operation. We'll use this in pathExistsInternal() to avoid checking
    // the same node twice, and again in updateHalfCriticalPath() to assert
    // there are no cycles.
    uint64_t m_seenAtGeneration = 0;

    // CONSTRUCTORS
    GraphPCNode() = મૂળભૂત;
    ~GraphPCNode() = મૂળભૂત;
};

//######################################################################
// GraphPathChecker implementation

template <GraphWay::en N_Way>
શૂન્ય GraphPathChecker::initHalfCriticalPaths(તાર્કિક checkOnly) {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay rev = way.invert();
    GraphStreamUnordered order(m_graphp, way);
    જ્યારે (અચલ V3GraphVertex* અચલ vertexp = order.nextp()) {
        unsigned critPathCost = 0;
        માટે (અચલ V3GraphEdge& edge : vertexp->edges<rev>()) {
            જો (!m_edgeFuncp(&edge)) ચાલુ;

            અચલ V3GraphVertex* wrelativep = edge.furtherp<rev>();
            અચલ GraphPCNode* અચલ wrelUserp = static_cast<GraphPCNode*>(wrelativep->userp());
            critPathCost = std::max(critPathCost, wrelUserp->m_cp[way] + 1);
        }

        GraphPCNode* અચલ ourUserp = static_cast<GraphPCNode*>(vertexp->userp());
        જો (checkOnly) {
            UASSERT_OBJ(ourUserp->m_cp[way] == critPathCost, vertexp,
                        "Validation of critical paths failed");
        } નહીંતર {
            ourUserp->m_cp[way] = critPathCost;
        }
    }
}

template શૂન્ય GraphPathChecker::initHalfCriticalPaths<GraphWay::FORWARD>(તાર્કિક);
template શૂન્ય GraphPathChecker::initHalfCriticalPaths<GraphWay::REVERSE>(તાર્કિક);

GraphPathChecker::GraphPathChecker(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
    : GraphAlg<V3Graph>{graphp, edgeFuncp} {
    માટે (V3GraphVertex& vtx : graphp->vertices()) {
        // Setup tracking structure for each node.  If delete a vertex
        // there would be a leak, but ok as accept only const V3Graph*'s.
        vtx.userp(નવું GraphPCNode);
    }
    // Init critical paths in userp() for each vertex
    initHalfCriticalPaths<GraphWay::FORWARD>(ખોટું);
    initHalfCriticalPaths<GraphWay::REVERSE>(ખોટું);
}

GraphPathChecker::~GraphPathChecker() {
    // Free every GraphPCNode
    માટે (V3GraphVertex& vtx : m_graphp->vertices()) {
        અચલ GraphPCNode* અચલ nodep = static_cast<GraphPCNode*>(vtx.userp());
        VL_DO_DANGLING(delete nodep, nodep);
        vtx.userp(nullptr);
    }
}

તાર્કિક GraphPathChecker::pathExistsInternal(અચલ V3GraphVertex* ap, અચલ V3GraphVertex* bp,
                                          unsigned* costp) {
    GraphPCNode* અચલ auserp = static_cast<GraphPCNode*>(ap->userp());
    અચલ GraphPCNode* અચલ buserp = static_cast<GraphPCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    જો (auserp->m_seenAtGeneration == m_generation) {
        જો (costp) *costp = 0;
        પાછા ખોટું;
    }
    auserp->m_seenAtGeneration = m_generation;

    જો (costp) *costp = 1;  // count 'a' toward the search cost

    જો (ap == bp) પાછા સાચું;

    // Rule out an a->b path based on their CPs
    જો (auserp->m_cp[GraphWay::REVERSE] < buserp->m_cp[GraphWay::REVERSE] + 1) પાછા ખોટું;
    જો (buserp->m_cp[GraphWay::FORWARD] < auserp->m_cp[GraphWay::FORWARD] + 1) પાછા ખોટું;

    // Slow path; visit some extended family
    તાર્કિક foundPath = ખોટું;
    માટે (અચલ V3GraphEdge& edge : ap->outEdges()) {
        જો (!m_edgeFuncp(&edge)) ચાલુ;

        unsigned childCost;
        જો (pathExistsInternal(edge.ટોચ(), bp, &childCost)) foundPath = સાચું;
        જો (costp) *costp += childCost;
    }

    પાછા foundPath;
}

તાર્કિક GraphPathChecker::pathExistsFrom(અચલ V3GraphVertex* fromp, અચલ V3GraphVertex* ટોચ) {
    incGeneration();
    પાછા pathExistsInternal(fromp, ટોચ);
}

તાર્કિક GraphPathChecker::isTransitiveEdge(અચલ V3GraphEdge* edgep) {
    અચલ V3GraphVertex* fromp = edgep->fromp();
    અચલ V3GraphVertex* ટોચ = edgep->ટોચ();
    incGeneration();
    માટે (અચલ V3GraphEdge& fromOut : fromp->outEdges()) {
        જો (&fromOut == edgep) ચાલુ;
        જો (pathExistsInternal(fromOut.ટોચ(), ટોચ)) પાછા સાચું;
    }
    પાછા ખોટું;
}
