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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3GraphPathChecker.h"

#অন্তর্ভাৱ "V3Global.h"
#অন্তর্ভাৱ "V3GraphStream.h"

//######################################################################
// GraphPCNode

সংরচনা GraphPCNode final {
    // User data for each node in GraphPathChecker.
    //
    // Like the LogicMTasks's, store before and after CPs for the nodes in
    // the GraphPathChecker graph.
    //
    // Unlike the LogicMTasks's, we have no cost info for the generic graph
    // accepted by GraphPathChecker, so assume each node has unit cost.
    std::array<uint32_t, GraphWay::অঙ্ক_WAYS> m_cp = {};

    // Detect if we've seen this node before in a given recursive
    // operation. We'll use this in pathExistsInternal() to avoid checking
    // the same node twice, and again in updateHalfCriticalPath() to assert
    // there are no cycles.
    uint64_t m_seenAtGeneration = 0;

    // CONSTRUCTORS
    GraphPCNode() = স্ৱভাৱ;
    ~GraphPCNode() = স্ৱভাৱ;
};

//######################################################################
// GraphPathChecker implementation

template <GraphWay::en N_Way>
শূন্য GraphPathChecker::initHalfCriticalPaths(তার্কিক checkOnly) {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay rev = way.invert();
    GraphStreamUnordered order(m_graphp, way);
    যেতিয়ালৈ (ধ্রুৱ V3GraphVertex* ধ্রুৱ vertexp = order.nextp()) {
        unsigned critPathCost = 0;
        বাবে (ধ্রুৱ V3GraphEdge& edge : vertexp->edges<rev>()) {
            যদি (!m_edgeFuncp(&edge)) অনুৱর্ত;

            ধ্রুৱ V3GraphVertex* wrelativep = edge.furtherp<rev>();
            ধ্রুৱ GraphPCNode* ধ্রুৱ wrelUserp = static_cast<GraphPCNode*>(wrelativep->userp());
            critPathCost = std::max(critPathCost, wrelUserp->m_cp[way] + 1);
        }

        GraphPCNode* ধ্রুৱ ourUserp = static_cast<GraphPCNode*>(vertexp->userp());
        যদি (checkOnly) {
            UASSERT_OBJ(ourUserp->m_cp[way] == critPathCost, vertexp,
                        "Validation of critical paths failed");
        } অন্যথা {
            ourUserp->m_cp[way] = critPathCost;
        }
    }
}

template শূন্য GraphPathChecker::initHalfCriticalPaths<GraphWay::FORWARD>(তার্কিক);
template শূন্য GraphPathChecker::initHalfCriticalPaths<GraphWay::REVERSE>(তার্কিক);

GraphPathChecker::GraphPathChecker(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
    : GraphAlg<V3Graph>{graphp, edgeFuncp} {
    বাবে (V3GraphVertex& vtx : graphp->vertices()) {
        // Setup tracking structure for each node.  If delete a vertex
        // there would be a leak, but ok as accept only const V3Graph*'s.
        vtx.userp(নূতন GraphPCNode);
    }
    // Init critical paths in userp() for each vertex
    initHalfCriticalPaths<GraphWay::FORWARD>(মিছা);
    initHalfCriticalPaths<GraphWay::REVERSE>(মিছা);
}

GraphPathChecker::~GraphPathChecker() {
    // Free every GraphPCNode
    বাবে (V3GraphVertex& vtx : m_graphp->vertices()) {
        ধ্রুৱ GraphPCNode* ধ্রুৱ nodep = static_cast<GraphPCNode*>(vtx.userp());
        VL_DO_DANGLING(delete nodep, nodep);
        vtx.userp(nullptr);
    }
}

তার্কিক GraphPathChecker::pathExistsInternal(ধ্রুৱ V3GraphVertex* ap, ধ্রুৱ V3GraphVertex* bp,
                                          unsigned* costp) {
    GraphPCNode* ধ্রুৱ auserp = static_cast<GraphPCNode*>(ap->userp());
    ধ্রুৱ GraphPCNode* ধ্রুৱ buserp = static_cast<GraphPCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    যদি (auserp->m_seenAtGeneration == m_generation) {
        যদি (costp) *costp = 0;
        ঘূৰাই মিছা;
    }
    auserp->m_seenAtGeneration = m_generation;

    যদি (costp) *costp = 1;  // count 'a' toward the search cost

    যদি (ap == bp) ঘূৰাই সঁচা;

    // Rule out an a->b path based on their CPs
    যদি (auserp->m_cp[GraphWay::REVERSE] < buserp->m_cp[GraphWay::REVERSE] + 1) ঘূৰাই মিছা;
    যদি (buserp->m_cp[GraphWay::FORWARD] < auserp->m_cp[GraphWay::FORWARD] + 1) ঘূৰাই মিছা;

    // Slow path; visit some extended family
    তার্কিক foundPath = মিছা;
    বাবে (ধ্রুৱ V3GraphEdge& edge : ap->outEdges()) {
        যদি (!m_edgeFuncp(&edge)) অনুৱর্ত;

        unsigned childCost;
        যদি (pathExistsInternal(edge.শীৰ্ষ(), bp, &childCost)) foundPath = সঁচা;
        যদি (costp) *costp += childCost;
    }

    ঘূৰাই foundPath;
}

তার্কিক GraphPathChecker::pathExistsFrom(ধ্রুৱ V3GraphVertex* fromp, ধ্রুৱ V3GraphVertex* শীৰ্ষ) {
    incGeneration();
    ঘূৰাই pathExistsInternal(fromp, শীৰ্ষ);
}

তার্কিক GraphPathChecker::isTransitiveEdge(ধ্রুৱ V3GraphEdge* edgep) {
    ধ্রুৱ V3GraphVertex* fromp = edgep->fromp();
    ধ্রুৱ V3GraphVertex* শীৰ্ষ = edgep->শীৰ্ষ();
    incGeneration();
    বাবে (ধ্রুৱ V3GraphEdge& fromOut : fromp->outEdges()) {
        যদি (&fromOut == edgep) অনুৱর্ত;
        যদি (pathExistsInternal(fromOut.শীৰ্ষ(), শীৰ্ষ)) ঘূৰাই সঁচা;
    }
    ঘূৰাই মিছা;
}
