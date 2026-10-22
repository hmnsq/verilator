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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3GraphPathChecker.h"

#ضمن "V3Global.h"
#ضمن "V3GraphStream.h"

//######################################################################
// GraphPCNode

بنية GraphPCNode final {
    // User data for each node in GraphPathChecker.
    //
    // Like the LogicMTasks's, store before and after CPs for the nodes in
    // the GraphPathChecker graph.
    //
    // Unlike the LogicMTasks's, we have no cost info for the generic graph
    // accepted by GraphPathChecker, so assume each node has unit cost.
    std::array<uint32_t, GraphWay::رقم_WAYS> m_cp = {};

    // Detect if we've seen this node before in a given recursive
    // operation. We'll use this in pathExistsInternal() to avoid checking
    // the same node twice, and again in updateHalfCriticalPath() to assert
    // there are no cycles.
    uint64_t m_seenAtGeneration = 0;

    // CONSTRUCTORS
    GraphPCNode() = افتراضي;
    ~GraphPCNode() = افتراضي;
};

//######################################################################
// GraphPathChecker implementation

template <GraphWay::en N_Way>
فراغ GraphPathChecker::initHalfCriticalPaths(منطقي checkOnly) {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay rev = way.invert();
    GraphStreamUnordered order(m_graphp, way);
    بينما (ثابت V3GraphVertex* ثابت vertexp = order.nextp()) {
        unsigned critPathCost = 0;
        لكل (ثابت V3GraphEdge& edge : vertexp->edges<rev>()) {
            إذا (!m_edgeFuncp(&edge)) استمر;

            ثابت V3GraphVertex* wrelativep = edge.furtherp<rev>();
            ثابت GraphPCNode* ثابت wrelUserp = static_cast<GraphPCNode*>(wrelativep->userp());
            critPathCost = std::max(critPathCost, wrelUserp->m_cp[way] + 1);
        }

        GraphPCNode* ثابت ourUserp = static_cast<GraphPCNode*>(vertexp->userp());
        إذا (checkOnly) {
            UASSERT_OBJ(ourUserp->m_cp[way] == critPathCost, vertexp,
                        "Validation of critical paths failed");
        } وإلا {
            ourUserp->m_cp[way] = critPathCost;
        }
    }
}

template فراغ GraphPathChecker::initHalfCriticalPaths<GraphWay::FORWARD>(منطقي);
template فراغ GraphPathChecker::initHalfCriticalPaths<GraphWay::REVERSE>(منطقي);

GraphPathChecker::GraphPathChecker(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
    : GraphAlg<V3Graph>{graphp, edgeFuncp} {
    لكل (V3GraphVertex& vtx : graphp->vertices()) {
        // Setup tracking structure for each node.  If delete a vertex
        // there would be a leak, but ok as accept only const V3Graph*'s.
        vtx.userp(جديد GraphPCNode);
    }
    // Init critical paths in userp() for each vertex
    initHalfCriticalPaths<GraphWay::FORWARD>(خطأ);
    initHalfCriticalPaths<GraphWay::REVERSE>(خطأ);
}

GraphPathChecker::~GraphPathChecker() {
    // Free every GraphPCNode
    لكل (V3GraphVertex& vtx : m_graphp->vertices()) {
        ثابت GraphPCNode* ثابت nodep = static_cast<GraphPCNode*>(vtx.userp());
        VL_DO_DANGLING(delete nodep, nodep);
        vtx.userp(nullptr);
    }
}

منطقي GraphPathChecker::pathExistsInternal(ثابت V3GraphVertex* ap, ثابت V3GraphVertex* bp,
                                          unsigned* costp) {
    GraphPCNode* ثابت auserp = static_cast<GraphPCNode*>(ap->userp());
    ثابت GraphPCNode* ثابت buserp = static_cast<GraphPCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    إذا (auserp->m_seenAtGeneration == m_generation) {
        إذا (costp) *costp = 0;
        إرجاع خطأ;
    }
    auserp->m_seenAtGeneration = m_generation;

    إذا (costp) *costp = 1;  // count 'a' toward the search cost

    إذا (ap == bp) إرجاع true;

    // Rule out an a->b path based on their CPs
    إذا (auserp->m_cp[GraphWay::REVERSE] < buserp->m_cp[GraphWay::REVERSE] + 1) إرجاع خطأ;
    إذا (buserp->m_cp[GraphWay::FORWARD] < auserp->m_cp[GraphWay::FORWARD] + 1) إرجاع خطأ;

    // Slow path; visit some extended family
    منطقي foundPath = خطأ;
    لكل (ثابت V3GraphEdge& edge : ap->outEdges()) {
        إذا (!m_edgeFuncp(&edge)) استمر;

        unsigned childCost;
        إذا (pathExistsInternal(edge.قمة(), bp, &childCost)) foundPath = true;
        إذا (costp) *costp += childCost;
    }

    إرجاع foundPath;
}

منطقي GraphPathChecker::pathExistsFrom(ثابت V3GraphVertex* fromp, ثابت V3GraphVertex* قمة) {
    incGeneration();
    إرجاع pathExistsInternal(fromp, قمة);
}

منطقي GraphPathChecker::isTransitiveEdge(ثابت V3GraphEdge* edgep) {
    ثابت V3GraphVertex* fromp = edgep->fromp();
    ثابت V3GraphVertex* قمة = edgep->قمة();
    incGeneration();
    لكل (ثابت V3GraphEdge& fromOut : fromp->outEdges()) {
        إذا (&fromOut == edgep) استمر;
        إذا (pathExistsInternal(fromOut.قمة(), قمة)) إرجاع true;
    }
    إرجاع خطأ;
}
