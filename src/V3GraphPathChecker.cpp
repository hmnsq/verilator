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
    std::array<uint32_t, GraphWay::अंक_WAYS> m_cp = {};

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

ढाँचा <GraphWay::en N_Way>
शून्य GraphPathChecker::initHalfCriticalPaths(तार्किक checkOnly) {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay rev = way.invert();
    GraphStreamUnordered order(m_graphp, way);
    यावत् (ध्रुव V3GraphVertex* ध्रुव vertexp = order.nextp()) {
        अचिन्हित critPathCost = 0;
        प्रति (ध्रुव V3GraphEdge& edge : vertexp->edges<rev>()) {
            यदि (!m_edgeFuncp(&edge)) अनुवर्त;

            ध्रुव V3GraphVertex* wrelativep = edge.furtherp<rev>();
            ध्रुव GraphPCNode* ध्रुव wrelUserp = static_cast<GraphPCNode*>(wrelativep->userp());
            critPathCost = std::max(critPathCost, wrelUserp->m_cp[way] + 1);
        }

        GraphPCNode* ध्रुव ourUserp = static_cast<GraphPCNode*>(vertexp->userp());
        यदि (checkOnly) {
            UASSERT_OBJ(ourUserp->m_cp[way] == critPathCost, vertexp,
                        "Validation of critical paths failed");
        } अन्यथा {
            ourUserp->m_cp[way] = critPathCost;
        }
    }
}

ढाँचा शून्य GraphPathChecker::initHalfCriticalPaths<GraphWay::FORWARD>(तार्किक);
ढाँचा शून्य GraphPathChecker::initHalfCriticalPaths<GraphWay::REVERSE>(तार्किक);

GraphPathChecker::GraphPathChecker(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
    : GraphAlg<V3Graph>{graphp, edgeFuncp} {
    प्रति (V3GraphVertex& vtx : graphp->vertices()) {
        // Setup tracking structure for each node.  If delete a vertex
        // there would be a leak, but ok as accept only const V3Graph*'s.
        vtx.userp(नूतन GraphPCNode);
    }
    // Init critical paths in userp() for each vertex
    initHalfCriticalPaths<GraphWay::FORWARD>(असत्य);
    initHalfCriticalPaths<GraphWay::REVERSE>(असत्य);
}

GraphPathChecker::~GraphPathChecker() {
    // Free every GraphPCNode
    प्रति (V3GraphVertex& vtx : m_graphp->vertices()) {
        ध्रुव GraphPCNode* ध्रुव nodep = static_cast<GraphPCNode*>(vtx.userp());
        VL_DO_DANGLING(delete nodep, nodep);
        vtx.userp(nullptr);
    }
}

तार्किक GraphPathChecker::pathExistsInternal(ध्रुव V3GraphVertex* ap, ध्रुव V3GraphVertex* bp,
                                          अचिन्हित* costp) {
    GraphPCNode* ध्रुव auserp = static_cast<GraphPCNode*>(ap->userp());
    ध्रुव GraphPCNode* ध्रुव buserp = static_cast<GraphPCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    यदि (auserp->m_seenAtGeneration == m_generation) {
        यदि (costp) *costp = 0;
        प्रत्यागम असत्य;
    }
    auserp->m_seenAtGeneration = m_generation;

    यदि (costp) *costp = 1;  // count 'a' toward the search cost

    यदि (ap == bp) प्रत्यागम सत्य;

    // Rule out an a->b path based on their CPs
    यदि (auserp->m_cp[GraphWay::REVERSE] < buserp->m_cp[GraphWay::REVERSE] + 1) प्रत्यागम असत्य;
    यदि (buserp->m_cp[GraphWay::FORWARD] < auserp->m_cp[GraphWay::FORWARD] + 1) प्रत्यागम असत्य;

    // Slow path; visit some extended family
    तार्किक foundPath = असत्य;
    प्रति (ध्रुव V3GraphEdge& edge : ap->outEdges()) {
        यदि (!m_edgeFuncp(&edge)) अनुवर्त;

        अचिन्हित childCost;
        यदि (pathExistsInternal(edge.शीर्ष(), bp, &childCost)) foundPath = सत्य;
        यदि (costp) *costp += childCost;
    }

    प्रत्यागम foundPath;
}

तार्किक GraphPathChecker::pathExistsFrom(ध्रुव V3GraphVertex* fromp, ध्रुव V3GraphVertex* शीर्ष) {
    incGeneration();
    प्रत्यागम pathExistsInternal(fromp, शीर्ष);
}

तार्किक GraphPathChecker::isTransitiveEdge(ध्रुव V3GraphEdge* edgep) {
    ध्रुव V3GraphVertex* fromp = edgep->fromp();
    ध्रुव V3GraphVertex* शीर्ष = edgep->शीर्ष();
    incGeneration();
    प्रति (ध्रुव V3GraphEdge& fromOut : fromp->outEdges()) {
        यदि (&fromOut == edgep) अनुवर्त;
        यदि (pathExistsInternal(fromOut.शीर्ष(), शीर्ष)) प्रत्यागम सत्य;
    }
    प्रत्यागम असत्य;
}
