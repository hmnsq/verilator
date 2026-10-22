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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3GraphPathChecker.h"

#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3GraphStream.h"

//######################################################################
// GraphPCNode

ಸಂರಚನಾ GraphPCNode final {
    // User data for each node in GraphPathChecker.
    //
    // Like the LogicMTasks's, store before and after CPs for the nodes in
    // the GraphPathChecker graph.
    //
    // Unlike the LogicMTasks's, we have no cost info for the generic graph
    // accepted by GraphPathChecker, so assume each node has unit cost.
    std::array<uint32_t, GraphWay::ಅಂಕೆ_WAYS> m_cp = {};

    // Detect if we've seen this node before in a given recursive
    // operation. We'll use this in pathExistsInternal() to avoid checking
    // the same node twice, and again in updateHalfCriticalPath() to assert
    // there are no cycles.
    uint64_t m_seenAtGeneration = 0;

    // CONSTRUCTORS
    GraphPCNode() = ಸ್ವಭಾವ;
    ~GraphPCNode() = ಸ್ವಭಾವ;
};

//######################################################################
// GraphPathChecker implementation

template <GraphWay::en N_Way>
ಶೂನ್ಯ GraphPathChecker::initHalfCriticalPaths(ತಾರ್ಕಿಕ checkOnly) {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay rev = way.invert();
    GraphStreamUnordered order(m_graphp, way);
    ಯಾವತ್ (ಧ್ರುವ V3GraphVertex* ಧ್ರುವ vertexp = order.nextp()) {
        unsigned critPathCost = 0;
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : vertexp->edges<rev>()) {
            ಯದಿ (!m_edgeFuncp(&edge)) ಅನುವರ್ತ;

            ಧ್ರುವ V3GraphVertex* wrelativep = edge.furtherp<rev>();
            ಧ್ರುವ GraphPCNode* ಧ್ರುವ wrelUserp = static_cast<GraphPCNode*>(wrelativep->userp());
            critPathCost = std::max(critPathCost, wrelUserp->m_cp[way] + 1);
        }

        GraphPCNode* ಧ್ರುವ ourUserp = static_cast<GraphPCNode*>(vertexp->userp());
        ಯದಿ (checkOnly) {
            UASSERT_OBJ(ourUserp->m_cp[way] == critPathCost, vertexp,
                        "Validation of critical paths failed");
        } ಅನ್ಯಥಾ {
            ourUserp->m_cp[way] = critPathCost;
        }
    }
}

template ಶೂನ್ಯ GraphPathChecker::initHalfCriticalPaths<GraphWay::FORWARD>(ತಾರ್ಕಿಕ);
template ಶೂನ್ಯ GraphPathChecker::initHalfCriticalPaths<GraphWay::REVERSE>(ತಾರ್ಕಿಕ);

GraphPathChecker::GraphPathChecker(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
    : GraphAlg<V3Graph>{graphp, edgeFuncp} {
    ಪ್ರತಿ (V3GraphVertex& vtx : graphp->vertices()) {
        // Setup tracking structure for each node.  If delete a vertex
        // there would be a leak, but ok as accept only const V3Graph*'s.
        vtx.userp(ನೂತನ GraphPCNode);
    }
    // Init critical paths in userp() for each vertex
    initHalfCriticalPaths<GraphWay::FORWARD>(ಅಸತ್ಯ);
    initHalfCriticalPaths<GraphWay::REVERSE>(ಅಸತ್ಯ);
}

GraphPathChecker::~GraphPathChecker() {
    // Free every GraphPCNode
    ಪ್ರತಿ (V3GraphVertex& vtx : m_graphp->vertices()) {
        ಧ್ರುವ GraphPCNode* ಧ್ರುವ nodep = static_cast<GraphPCNode*>(vtx.userp());
        VL_DO_DANGLING(delete nodep, nodep);
        vtx.userp(nullptr);
    }
}

ತಾರ್ಕಿಕ GraphPathChecker::pathExistsInternal(ಧ್ರುವ V3GraphVertex* ap, ಧ್ರುವ V3GraphVertex* bp,
                                          unsigned* costp) {
    GraphPCNode* ಧ್ರುವ auserp = static_cast<GraphPCNode*>(ap->userp());
    ಧ್ರುವ GraphPCNode* ಧ್ರುವ buserp = static_cast<GraphPCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    ಯದಿ (auserp->m_seenAtGeneration == m_generation) {
        ಯದಿ (costp) *costp = 0;
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    auserp->m_seenAtGeneration = m_generation;

    ಯದಿ (costp) *costp = 1;  // count 'a' toward the search cost

    ಯದಿ (ap == bp) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;

    // Rule out an a->b path based on their CPs
    ಯದಿ (auserp->m_cp[GraphWay::REVERSE] < buserp->m_cp[GraphWay::REVERSE] + 1) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಯದಿ (buserp->m_cp[GraphWay::FORWARD] < auserp->m_cp[GraphWay::FORWARD] + 1) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

    // Slow path; visit some extended family
    ತಾರ್ಕಿಕ foundPath = ಅಸತ್ಯ;
    ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : ap->outEdges()) {
        ಯದಿ (!m_edgeFuncp(&edge)) ಅನುವರ್ತ;

        unsigned childCost;
        ಯದಿ (pathExistsInternal(edge.ತುದಿ(), bp, &childCost)) foundPath = ಸತ್ಯ;
        ಯದಿ (costp) *costp += childCost;
    }

    ಪ್ರತ್ಯಾಗಮ foundPath;
}

ತಾರ್ಕಿಕ GraphPathChecker::pathExistsFrom(ಧ್ರುವ V3GraphVertex* fromp, ಧ್ರುವ V3GraphVertex* ತುದಿ) {
    incGeneration();
    ಪ್ರತ್ಯಾಗಮ pathExistsInternal(fromp, ತುದಿ);
}

ತಾರ್ಕಿಕ GraphPathChecker::isTransitiveEdge(ಧ್ರುವ V3GraphEdge* edgep) {
    ಧ್ರುವ V3GraphVertex* fromp = edgep->fromp();
    ಧ್ರುವ V3GraphVertex* ತುದಿ = edgep->ತುದಿ();
    incGeneration();
    ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& fromOut : fromp->outEdges()) {
        ಯದಿ (&fromOut == edgep) ಅನುವರ್ತ;
        ಯದಿ (pathExistsInternal(fromOut.ತುದಿ(), ತುದಿ)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
}
