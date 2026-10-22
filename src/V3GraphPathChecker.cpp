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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3GraphPathChecker.h"

#অন্তর্ভাব "V3Global.h"
#অন্তর্ভাব "V3GraphStream.h"

//######################################################################
// GraphPCNode

কাঠামো GraphPCNode final {
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
    GraphPCNode() = পূর্বনির্ধারিত;
    ~GraphPCNode() = পূর্বনির্ধারিত;
};

//######################################################################
// GraphPathChecker implementation

template <GraphWay::en N_Way>
শূন্য GraphPathChecker::initHalfCriticalPaths(যৌক্তিক checkOnly) {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay rev = way.invert();
    GraphStreamUnordered order(m_graphp, way);
    যখন (ধ্রুবক V3GraphVertex* ধ্রুবক vertexp = order.nextp()) {
        unsigned critPathCost = 0;
        জন্য (ধ্রুবক V3GraphEdge& edge : vertexp->edges<rev>()) {
            যদি (!m_edgeFuncp(&edge)) চালু;

            ধ্রুবক V3GraphVertex* wrelativep = edge.furtherp<rev>();
            ধ্রুবক GraphPCNode* ধ্রুবক wrelUserp = static_cast<GraphPCNode*>(wrelativep->userp());
            critPathCost = std::max(critPathCost, wrelUserp->m_cp[way] + 1);
        }

        GraphPCNode* ধ্রুবক ourUserp = static_cast<GraphPCNode*>(vertexp->userp());
        যদি (checkOnly) {
            UASSERT_OBJ(ourUserp->m_cp[way] == critPathCost, vertexp,
                        "Validation of critical paths failed");
        } অন্যথা {
            ourUserp->m_cp[way] = critPathCost;
        }
    }
}

template শূন্য GraphPathChecker::initHalfCriticalPaths<GraphWay::FORWARD>(যৌক্তিক);
template শূন্য GraphPathChecker::initHalfCriticalPaths<GraphWay::REVERSE>(যৌক্তিক);

GraphPathChecker::GraphPathChecker(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
    : GraphAlg<V3Graph>{graphp, edgeFuncp} {
    জন্য (V3GraphVertex& vtx : graphp->vertices()) {
        // Setup tracking structure for each node.  If delete a vertex
        // there would be a leak, but ok as accept only const V3Graph*'s.
        vtx.userp(নতুন GraphPCNode);
    }
    // Init critical paths in userp() for each vertex
    initHalfCriticalPaths<GraphWay::FORWARD>(মিথ্যা);
    initHalfCriticalPaths<GraphWay::REVERSE>(মিথ্যা);
}

GraphPathChecker::~GraphPathChecker() {
    // Free every GraphPCNode
    জন্য (V3GraphVertex& vtx : m_graphp->vertices()) {
        ধ্রুবক GraphPCNode* ধ্রুবক nodep = static_cast<GraphPCNode*>(vtx.userp());
        VL_DO_DANGLING(delete nodep, nodep);
        vtx.userp(nullptr);
    }
}

যৌক্তিক GraphPathChecker::pathExistsInternal(ধ্রুবক V3GraphVertex* ap, ধ্রুবক V3GraphVertex* bp,
                                          unsigned* costp) {
    GraphPCNode* ধ্রুবক auserp = static_cast<GraphPCNode*>(ap->userp());
    ধ্রুবক GraphPCNode* ধ্রুবক buserp = static_cast<GraphPCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    যদি (auserp->m_seenAtGeneration == m_generation) {
        যদি (costp) *costp = 0;
        ফেরত মিথ্যা;
    }
    auserp->m_seenAtGeneration = m_generation;

    যদি (costp) *costp = 1;  // count 'a' toward the search cost

    যদি (ap == bp) ফেরত সত্য;

    // Rule out an a->b path based on their CPs
    যদি (auserp->m_cp[GraphWay::REVERSE] < buserp->m_cp[GraphWay::REVERSE] + 1) ফেরত মিথ্যা;
    যদি (buserp->m_cp[GraphWay::FORWARD] < auserp->m_cp[GraphWay::FORWARD] + 1) ফেরত মিথ্যা;

    // Slow path; visit some extended family
    যৌক্তিক foundPath = মিথ্যা;
    জন্য (ধ্রুবক V3GraphEdge& edge : ap->outEdges()) {
        যদি (!m_edgeFuncp(&edge)) চালু;

        unsigned childCost;
        যদি (pathExistsInternal(edge.শীর্ষ(), bp, &childCost)) foundPath = সত্য;
        যদি (costp) *costp += childCost;
    }

    ফেরত foundPath;
}

যৌক্তিক GraphPathChecker::pathExistsFrom(ধ্রুবক V3GraphVertex* fromp, ধ্রুবক V3GraphVertex* শীর্ষ) {
    incGeneration();
    ফেরত pathExistsInternal(fromp, শীর্ষ);
}

যৌক্তিক GraphPathChecker::isTransitiveEdge(ধ্রুবক V3GraphEdge* edgep) {
    ধ্রুবক V3GraphVertex* fromp = edgep->fromp();
    ধ্রুবক V3GraphVertex* শীর্ষ = edgep->শীর্ষ();
    incGeneration();
    জন্য (ধ্রুবক V3GraphEdge& fromOut : fromp->outEdges()) {
        যদি (&fromOut == edgep) চালু;
        যদি (pathExistsInternal(fromOut.শীর্ষ(), শীর্ষ)) ফেরত সত্য;
    }
    ফেরত মিথ্যা;
}
