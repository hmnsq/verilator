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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3GraphPathChecker.h"

#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3GraphStream.h"

//######################################################################
// GraphPCNode

கட்டமைப்பு GraphPCNode final {
    // User data for each node in GraphPathChecker.
    //
    // Like the LogicMTasks's, store before and after CPs for the nodes in
    // the GraphPathChecker graph.
    //
    // Unlike the LogicMTasks's, we have no cost info for the generic graph
    // accepted by GraphPathChecker, so assume each node has unit cost.
    std::array<uint32_t, GraphWay::இலக்கம்_WAYS> m_cp = {};

    // Detect if we've seen this node before in a given recursive
    // operation. We'll use this in pathExistsInternal() to avoid checking
    // the same node twice, and again in updateHalfCriticalPath() to assert
    // there are no cycles.
    uint64_t m_seenAtGeneration = 0;

    // CONSTRUCTORS
    GraphPCNode() = இயல்பு;
    ~GraphPCNode() = இயல்பு;
};

//######################################################################
// GraphPathChecker implementation

template <GraphWay::en N_Way>
வெறுமை GraphPathChecker::initHalfCriticalPaths(தருக்க checkOnly) {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay rev = way.invert();
    GraphStreamUnordered order(m_graphp, way);
    போது (மாறிலி V3GraphVertex* மாறிலி vertexp = order.nextp()) {
        unsigned critPathCost = 0;
        க்கு (மாறிலி V3GraphEdge& edge : vertexp->edges<rev>()) {
            எனில் (!m_edgeFuncp(&edge)) தொடர்;

            மாறிலி V3GraphVertex* wrelativep = edge.furtherp<rev>();
            மாறிலி GraphPCNode* மாறிலி wrelUserp = static_cast<GraphPCNode*>(wrelativep->userp());
            critPathCost = std::max(critPathCost, wrelUserp->m_cp[way] + 1);
        }

        GraphPCNode* மாறிலி ourUserp = static_cast<GraphPCNode*>(vertexp->userp());
        எனில் (checkOnly) {
            UASSERT_OBJ(ourUserp->m_cp[way] == critPathCost, vertexp,
                        "Validation of critical paths failed");
        } இல்லையெனில் {
            ourUserp->m_cp[way] = critPathCost;
        }
    }
}

template வெறுமை GraphPathChecker::initHalfCriticalPaths<GraphWay::FORWARD>(தருக்க);
template வெறுமை GraphPathChecker::initHalfCriticalPaths<GraphWay::REVERSE>(தருக்க);

GraphPathChecker::GraphPathChecker(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
    : GraphAlg<V3Graph>{graphp, edgeFuncp} {
    க்கு (V3GraphVertex& vtx : graphp->vertices()) {
        // Setup tracking structure for each node.  If delete a vertex
        // there would be a leak, but ok as accept only const V3Graph*'s.
        vtx.userp(புதிய GraphPCNode);
    }
    // Init critical paths in userp() for each vertex
    initHalfCriticalPaths<GraphWay::FORWARD>(பொய்);
    initHalfCriticalPaths<GraphWay::REVERSE>(பொய்);
}

GraphPathChecker::~GraphPathChecker() {
    // Free every GraphPCNode
    க்கு (V3GraphVertex& vtx : m_graphp->vertices()) {
        மாறிலி GraphPCNode* மாறிலி nodep = static_cast<GraphPCNode*>(vtx.userp());
        VL_DO_DANGLING(delete nodep, nodep);
        vtx.userp(nullptr);
    }
}

தருக்க GraphPathChecker::pathExistsInternal(மாறிலி V3GraphVertex* ap, மாறிலி V3GraphVertex* bp,
                                          unsigned* costp) {
    GraphPCNode* மாறிலி auserp = static_cast<GraphPCNode*>(ap->userp());
    மாறிலி GraphPCNode* மாறிலி buserp = static_cast<GraphPCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    எனில் (auserp->m_seenAtGeneration == m_generation) {
        எனில் (costp) *costp = 0;
        திரும்பு பொய்;
    }
    auserp->m_seenAtGeneration = m_generation;

    எனில் (costp) *costp = 1;  // count 'a' toward the search cost

    எனில் (ap == bp) திரும்பு உண்மை;

    // Rule out an a->b path based on their CPs
    எனில் (auserp->m_cp[GraphWay::REVERSE] < buserp->m_cp[GraphWay::REVERSE] + 1) திரும்பு பொய்;
    எனில் (buserp->m_cp[GraphWay::FORWARD] < auserp->m_cp[GraphWay::FORWARD] + 1) திரும்பு பொய்;

    // Slow path; visit some extended family
    தருக்க foundPath = பொய்;
    க்கு (மாறிலி V3GraphEdge& edge : ap->outEdges()) {
        எனில் (!m_edgeFuncp(&edge)) தொடர்;

        unsigned childCost;
        எனில் (pathExistsInternal(edge.உச்சி(), bp, &childCost)) foundPath = உண்மை;
        எனில் (costp) *costp += childCost;
    }

    திரும்பு foundPath;
}

தருக்க GraphPathChecker::pathExistsFrom(மாறிலி V3GraphVertex* fromp, மாறிலி V3GraphVertex* உச்சி) {
    incGeneration();
    திரும்பு pathExistsInternal(fromp, உச்சி);
}

தருக்க GraphPathChecker::isTransitiveEdge(மாறிலி V3GraphEdge* edgep) {
    மாறிலி V3GraphVertex* fromp = edgep->fromp();
    மாறிலி V3GraphVertex* உச்சி = edgep->உச்சி();
    incGeneration();
    க்கு (மாறிலி V3GraphEdge& fromOut : fromp->outEdges()) {
        எனில் (&fromOut == edgep) தொடர்;
        எனில் (pathExistsInternal(fromOut.உச்சி(), உச்சி)) திரும்பு உண்மை;
    }
    திரும்பு பொய்;
}
