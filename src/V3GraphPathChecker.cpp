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
    GraphPCNode() = স্বভাব;
    ~GraphPCNode() = স্বভাব;
};

//######################################################################
// GraphPathChecker implementation

template <GraphWay::en N_Way>
শূন্য GraphPathChecker::initHalfCriticalPaths(তার্কিক checkOnly) {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay rev = way.invert();
    GraphStreamUnordered order(m_graphp, way);
    যাবত্ (ধ্রুব V3GraphVertex* ধ্রুব vertexp = order.nextp()) {
        unsigned critPathCost = 0;
        প্রতি (ধ্রুব V3GraphEdge& edge : vertexp->edges<rev>()) {
            যদি (!m_edgeFuncp(&edge)) অনুবর্ত;

            ধ্রুব V3GraphVertex* wrelativep = edge.furtherp<rev>();
            ধ্রুব GraphPCNode* ধ্রুব wrelUserp = static_cast<GraphPCNode*>(wrelativep->userp());
            critPathCost = std::max(critPathCost, wrelUserp->m_cp[way] + 1);
        }

        GraphPCNode* ধ্রুব ourUserp = static_cast<GraphPCNode*>(vertexp->userp());
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
    প্রতি (V3GraphVertex& vtx : graphp->vertices()) {
        // Setup tracking structure for each node.  If delete a vertex
        // there would be a leak, but ok as accept only const V3Graph*'s.
        vtx.userp(নূতন GraphPCNode);
    }
    // Init critical paths in userp() for each vertex
    initHalfCriticalPaths<GraphWay::FORWARD>(অসত্য);
    initHalfCriticalPaths<GraphWay::REVERSE>(অসত্য);
}

GraphPathChecker::~GraphPathChecker() {
    // Free every GraphPCNode
    প্রতি (V3GraphVertex& vtx : m_graphp->vertices()) {
        ধ্রুব GraphPCNode* ধ্রুব nodep = static_cast<GraphPCNode*>(vtx.userp());
        VL_DO_DANGLING(delete nodep, nodep);
        vtx.userp(nullptr);
    }
}

তার্কিক GraphPathChecker::pathExistsInternal(ধ্রুব V3GraphVertex* ap, ধ্রুব V3GraphVertex* bp,
                                          unsigned* costp) {
    GraphPCNode* ধ্রুব auserp = static_cast<GraphPCNode*>(ap->userp());
    ধ্রুব GraphPCNode* ধ্রুব buserp = static_cast<GraphPCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    যদি (auserp->m_seenAtGeneration == m_generation) {
        যদি (costp) *costp = 0;
        প্রত্যাগম অসত্য;
    }
    auserp->m_seenAtGeneration = m_generation;

    যদি (costp) *costp = 1;  // count 'a' toward the search cost

    যদি (ap == bp) প্রত্যাগম সত্য;

    // Rule out an a->b path based on their CPs
    যদি (auserp->m_cp[GraphWay::REVERSE] < buserp->m_cp[GraphWay::REVERSE] + 1) প্রত্যাগম অসত্য;
    যদি (buserp->m_cp[GraphWay::FORWARD] < auserp->m_cp[GraphWay::FORWARD] + 1) প্রত্যাগম অসত্য;

    // Slow path; visit some extended family
    তার্কিক foundPath = অসত্য;
    প্রতি (ধ্রুব V3GraphEdge& edge : ap->outEdges()) {
        যদি (!m_edgeFuncp(&edge)) অনুবর্ত;

        unsigned childCost;
        যদি (pathExistsInternal(edge.শীর্ষ(), bp, &childCost)) foundPath = সত্য;
        যদি (costp) *costp += childCost;
    }

    প্রত্যাগম foundPath;
}

তার্কিক GraphPathChecker::pathExistsFrom(ধ্রুব V3GraphVertex* fromp, ধ্রুব V3GraphVertex* শীর্ষ) {
    incGeneration();
    প্রত্যাগম pathExistsInternal(fromp, শীর্ষ);
}

তার্কিক GraphPathChecker::isTransitiveEdge(ধ্রুব V3GraphEdge* edgep) {
    ধ্রুব V3GraphVertex* fromp = edgep->fromp();
    ধ্রুব V3GraphVertex* শীর্ষ = edgep->শীর্ষ();
    incGeneration();
    প্রতি (ধ্রুব V3GraphEdge& fromOut : fromp->outEdges()) {
        যদি (&fromOut == edgep) অনুবর্ত;
        যদি (pathExistsInternal(fromOut.শীর্ষ(), শীর্ষ)) প্রত্যাগম সত্য;
    }
    প্রত্যাগম অসত্য;
}
