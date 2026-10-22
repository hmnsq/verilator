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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3GraphPathChecker.h"

#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3GraphStream.h"

//######################################################################
// GraphPCNode

ସଂରଚନା GraphPCNode final {
    // User data for each node in GraphPathChecker.
    //
    // Like the LogicMTasks's, store before and after CPs for the nodes in
    // the GraphPathChecker graph.
    //
    // Unlike the LogicMTasks's, we have no cost info for the generic graph
    // accepted by GraphPathChecker, so assume each node has unit cost.
    std::array<uint32_t, GraphWay::ଅଙ୍କ_WAYS> m_cp = {};

    // Detect if we've seen this node before in a given recursive
    // operation. We'll use this in pathExistsInternal() to avoid checking
    // the same node twice, and again in updateHalfCriticalPath() to assert
    // there are no cycles.
    uint64_t m_seenAtGeneration = 0;

    // CONSTRUCTORS
    GraphPCNode() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~GraphPCNode() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// GraphPathChecker implementation

template <GraphWay::en N_Way>
ଶୂନ୍ୟ GraphPathChecker::initHalfCriticalPaths(ତାର୍କିକ checkOnly) {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay rev = way.invert();
    GraphStreamUnordered order(m_graphp, way);
    ଯେତେବେଳେ (ଅଚଳ V3GraphVertex* ଅଚଳ vertexp = order.nextp()) {
        unsigned critPathCost = 0;
        ପାଇଁ (ଅଚଳ V3GraphEdge& edge : vertexp->edges<rev>()) {
            ଯଦି (!m_edgeFuncp(&edge)) ଜାରି;

            ଅଚଳ V3GraphVertex* wrelativep = edge.furtherp<rev>();
            ଅଚଳ GraphPCNode* ଅଚଳ wrelUserp = static_cast<GraphPCNode*>(wrelativep->userp());
            critPathCost = std::max(critPathCost, wrelUserp->m_cp[way] + 1);
        }

        GraphPCNode* ଅଚଳ ourUserp = static_cast<GraphPCNode*>(vertexp->userp());
        ଯଦି (checkOnly) {
            UASSERT_OBJ(ourUserp->m_cp[way] == critPathCost, vertexp,
                        "Validation of critical paths failed");
        } ଅନ୍ୟଥା {
            ourUserp->m_cp[way] = critPathCost;
        }
    }
}

template ଶୂନ୍ୟ GraphPathChecker::initHalfCriticalPaths<GraphWay::FORWARD>(ତାର୍କିକ);
template ଶୂନ୍ୟ GraphPathChecker::initHalfCriticalPaths<GraphWay::REVERSE>(ତାର୍କିକ);

GraphPathChecker::GraphPathChecker(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
    : GraphAlg<V3Graph>{graphp, edgeFuncp} {
    ପାଇଁ (V3GraphVertex& vtx : graphp->vertices()) {
        // Setup tracking structure for each node.  If delete a vertex
        // there would be a leak, but ok as accept only const V3Graph*'s.
        vtx.userp(ନୂତନ GraphPCNode);
    }
    // Init critical paths in userp() for each vertex
    initHalfCriticalPaths<GraphWay::FORWARD>(ମିଥ୍ୟା);
    initHalfCriticalPaths<GraphWay::REVERSE>(ମିଥ୍ୟା);
}

GraphPathChecker::~GraphPathChecker() {
    // Free every GraphPCNode
    ପାଇଁ (V3GraphVertex& vtx : m_graphp->vertices()) {
        ଅଚଳ GraphPCNode* ଅଚଳ nodep = static_cast<GraphPCNode*>(vtx.userp());
        VL_DO_DANGLING(delete nodep, nodep);
        vtx.userp(nullptr);
    }
}

ତାର୍କିକ GraphPathChecker::pathExistsInternal(ଅଚଳ V3GraphVertex* ap, ଅଚଳ V3GraphVertex* bp,
                                          unsigned* costp) {
    GraphPCNode* ଅଚଳ auserp = static_cast<GraphPCNode*>(ap->userp());
    ଅଚଳ GraphPCNode* ଅଚଳ buserp = static_cast<GraphPCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    ଯଦି (auserp->m_seenAtGeneration == m_generation) {
        ଯଦି (costp) *costp = 0;
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    auserp->m_seenAtGeneration = m_generation;

    ଯଦି (costp) *costp = 1;  // count 'a' toward the search cost

    ଯଦି (ap == bp) ଫେରନ୍ତୁ ସତ୍ୟ;

    // Rule out an a->b path based on their CPs
    ଯଦି (auserp->m_cp[GraphWay::REVERSE] < buserp->m_cp[GraphWay::REVERSE] + 1) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଯଦି (buserp->m_cp[GraphWay::FORWARD] < auserp->m_cp[GraphWay::FORWARD] + 1) ଫେରନ୍ତୁ ମିଥ୍ୟା;

    // Slow path; visit some extended family
    ତାର୍କିକ foundPath = ମିଥ୍ୟା;
    ପାଇଁ (ଅଚଳ V3GraphEdge& edge : ap->outEdges()) {
        ଯଦି (!m_edgeFuncp(&edge)) ଜାରି;

        unsigned childCost;
        ଯଦି (pathExistsInternal(edge.ଶୀର୍ଷ(), bp, &childCost)) foundPath = ସତ୍ୟ;
        ଯଦି (costp) *costp += childCost;
    }

    ଫେରନ୍ତୁ foundPath;
}

ତାର୍କିକ GraphPathChecker::pathExistsFrom(ଅଚଳ V3GraphVertex* fromp, ଅଚଳ V3GraphVertex* ଶୀର୍ଷ) {
    incGeneration();
    ଫେରନ୍ତୁ pathExistsInternal(fromp, ଶୀର୍ଷ);
}

ତାର୍କିକ GraphPathChecker::isTransitiveEdge(ଅଚଳ V3GraphEdge* edgep) {
    ଅଚଳ V3GraphVertex* fromp = edgep->fromp();
    ଅଚଳ V3GraphVertex* ଶୀର୍ଷ = edgep->ଶୀର୍ଷ();
    incGeneration();
    ପାଇଁ (ଅଚଳ V3GraphEdge& fromOut : fromp->outEdges()) {
        ଯଦି (&fromOut == edgep) ଜାରି;
        ଯଦି (pathExistsInternal(fromOut.ଶୀର୍ଷ(), ଶୀର୍ଷ)) ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
}
