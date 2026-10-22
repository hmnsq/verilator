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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3GraphPathChecker.h"

#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3GraphStream.h"

//######################################################################
// GraphPCNode

संरचना GraphPCNode final {
    // User data for each node in GraphPathChecker.
    //
    // Like the LogicMTasks's, store before and after CPs for the nodes in
    // the GraphPathChecker graph.
    //
    // Unlike the LogicMTasks's, we have no cost info for the generic graph
    // accepted by GraphPathChecker, so assume each node has unit cost.
    std::array<uint32_t, GraphWay::अङ्कः_WAYS> m_cp = {};

    // Detect if we've seen this node before in a given recursive
    // operation. We'll use this in pathExistsInternal() to avoid checking
    // the same node twice, and again in updateHalfCriticalPath() to assert
    // there are no cycles.
    uint64_t m_seenAtGeneration = 0;

    // CONSTRUCTORS
    GraphPCNode() = स्वभाव;
    ~GraphPCNode() = स्वभाव;
};

//######################################################################
// GraphPathChecker implementation

template <GraphWay::en N_Way>
शून्य GraphPathChecker::initHalfCriticalPaths(तार्किक checkOnly) {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay rev = way.invert();
    GraphStreamUnordered order(m_graphp, way);
    याव (ध्रुव V3GraphVertex* ध्रुव vertexp = order.nextp()) {
        unsigned critPathCost = 0;
        अत्थाय (ध्रुव V3GraphEdge& edge : vertexp->edges<rev>()) {
            सचे (!m_edgeFuncp(&edge)) अनुवर्त;

            ध्रुव V3GraphVertex* wrelativep = edge.furtherp<rev>();
            ध्रुव GraphPCNode* ध्रुव wrelUserp = static_cast<GraphPCNode*>(wrelativep->userp());
            critPathCost = std::max(critPathCost, wrelUserp->m_cp[way] + 1);
        }

        GraphPCNode* ध्रुव ourUserp = static_cast<GraphPCNode*>(vertexp->userp());
        सचे (checkOnly) {
            UASSERT_OBJ(ourUserp->m_cp[way] == critPathCost, vertexp,
                        "Validation of critical paths failed");
        } अञ्ञथा {
            ourUserp->m_cp[way] = critPathCost;
        }
    }
}

template शून्य GraphPathChecker::initHalfCriticalPaths<GraphWay::FORWARD>(तार्किक);
template शून्य GraphPathChecker::initHalfCriticalPaths<GraphWay::REVERSE>(तार्किक);

GraphPathChecker::GraphPathChecker(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
    : GraphAlg<V3Graph>{graphp, edgeFuncp} {
    अत्थाय (V3GraphVertex& vtx : graphp->vertices()) {
        // Setup tracking structure for each node.  If delete a vertex
        // there would be a leak, but ok as accept only const V3Graph*'s.
        vtx.userp(नूतन GraphPCNode);
    }
    // Init critical paths in userp() for each vertex
    initHalfCriticalPaths<GraphWay::FORWARD>(मिच्छा);
    initHalfCriticalPaths<GraphWay::REVERSE>(मिच्छा);
}

GraphPathChecker::~GraphPathChecker() {
    // Free every GraphPCNode
    अत्थाय (V3GraphVertex& vtx : m_graphp->vertices()) {
        ध्रुव GraphPCNode* ध्रुव nodep = static_cast<GraphPCNode*>(vtx.userp());
        VL_DO_DANGLING(delete nodep, nodep);
        vtx.userp(nullptr);
    }
}

तार्किक GraphPathChecker::pathExistsInternal(ध्रुव V3GraphVertex* ap, ध्रुव V3GraphVertex* bp,
                                          unsigned* costp) {
    GraphPCNode* ध्रुव auserp = static_cast<GraphPCNode*>(ap->userp());
    ध्रुव GraphPCNode* ध्रुव buserp = static_cast<GraphPCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    सचे (auserp->m_seenAtGeneration == m_generation) {
        सचे (costp) *costp = 0;
        पटिनिवत्त मिच्छा;
    }
    auserp->m_seenAtGeneration = m_generation;

    सचे (costp) *costp = 1;  // count 'a' toward the search cost

    सचे (ap == bp) पटिनिवत्त सच्च;

    // Rule out an a->b path based on their CPs
    सचे (auserp->m_cp[GraphWay::REVERSE] < buserp->m_cp[GraphWay::REVERSE] + 1) पटिनिवत्त मिच्छा;
    सचे (buserp->m_cp[GraphWay::FORWARD] < auserp->m_cp[GraphWay::FORWARD] + 1) पटिनिवत्त मिच्छा;

    // Slow path; visit some extended family
    तार्किक foundPath = मिच्छा;
    अत्थाय (ध्रुव V3GraphEdge& edge : ap->outEdges()) {
        सचे (!m_edgeFuncp(&edge)) अनुवर्त;

        unsigned childCost;
        सचे (pathExistsInternal(edge.शिखरम्(), bp, &childCost)) foundPath = सच्च;
        सचे (costp) *costp += childCost;
    }

    पटिनिवत्त foundPath;
}

तार्किक GraphPathChecker::pathExistsFrom(ध्रुव V3GraphVertex* fromp, ध्रुव V3GraphVertex* शिखरम्) {
    incGeneration();
    पटिनिवत्त pathExistsInternal(fromp, शिखरम्);
}

तार्किक GraphPathChecker::isTransitiveEdge(ध्रुव V3GraphEdge* edgep) {
    ध्रुव V3GraphVertex* fromp = edgep->fromp();
    ध्रुव V3GraphVertex* शिखरम् = edgep->शिखरम्();
    incGeneration();
    अत्थाय (ध्रुव V3GraphEdge& fromOut : fromp->outEdges()) {
        सचे (&fromOut == edgep) अनुवर्त;
        सचे (pathExistsInternal(fromOut.शिखरम्(), शिखरम्)) पटिनिवत्त सच्च;
    }
    पटिनिवत्त मिच्छा;
}
