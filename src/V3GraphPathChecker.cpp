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
    GraphPCNode() = पूर्वनिर्धारित;
    ~GraphPCNode() = पूर्वनिर्धारित;
};

//######################################################################
// GraphPathChecker implementation

ढाँचा <GraphWay::en N_Way>
शून्य GraphPathChecker::initHalfCriticalPaths(तार्किक checkOnly) {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay rev = way.invert();
    GraphStreamUnordered order(m_graphp, way);
    पर्यंत (स्थिरांक V3GraphVertex* स्थिरांक vertexp = order.nextp()) {
        अचिन्हित critPathCost = 0;
        साठी (स्थिरांक V3GraphEdge& edge : vertexp->edges<rev>()) {
            जर (!m_edgeFuncp(&edge)) सुरूठेवा;

            स्थिरांक V3GraphVertex* wrelativep = edge.furtherp<rev>();
            स्थिरांक GraphPCNode* स्थिरांक wrelUserp = static_cast<GraphPCNode*>(wrelativep->userp());
            critPathCost = std::max(critPathCost, wrelUserp->m_cp[way] + 1);
        }

        GraphPCNode* स्थिरांक ourUserp = static_cast<GraphPCNode*>(vertexp->userp());
        जर (checkOnly) {
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
    साठी (V3GraphVertex& vtx : graphp->vertices()) {
        // Setup tracking structure for each node.  If delete a vertex
        // there would be a leak, but ok as accept only const V3Graph*'s.
        vtx.userp(नवीन GraphPCNode);
    }
    // Init critical paths in userp() for each vertex
    initHalfCriticalPaths<GraphWay::FORWARD>(असत्य);
    initHalfCriticalPaths<GraphWay::REVERSE>(असत्य);
}

GraphPathChecker::~GraphPathChecker() {
    // Free every GraphPCNode
    साठी (V3GraphVertex& vtx : m_graphp->vertices()) {
        स्थिरांक GraphPCNode* स्थिरांक nodep = static_cast<GraphPCNode*>(vtx.userp());
        VL_DO_DANGLING(delete nodep, nodep);
        vtx.userp(nullptr);
    }
}

तार्किक GraphPathChecker::pathExistsInternal(स्थिरांक V3GraphVertex* ap, स्थिरांक V3GraphVertex* bp,
                                          अचिन्हित* costp) {
    GraphPCNode* स्थिरांक auserp = static_cast<GraphPCNode*>(ap->userp());
    स्थिरांक GraphPCNode* स्थिरांक buserp = static_cast<GraphPCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    जर (auserp->m_seenAtGeneration == m_generation) {
        जर (costp) *costp = 0;
        परत असत्य;
    }
    auserp->m_seenAtGeneration = m_generation;

    जर (costp) *costp = 1;  // count 'a' toward the search cost

    जर (ap == bp) परत सत्य;

    // Rule out an a->b path based on their CPs
    जर (auserp->m_cp[GraphWay::REVERSE] < buserp->m_cp[GraphWay::REVERSE] + 1) परत असत्य;
    जर (buserp->m_cp[GraphWay::FORWARD] < auserp->m_cp[GraphWay::FORWARD] + 1) परत असत्य;

    // Slow path; visit some extended family
    तार्किक foundPath = असत्य;
    साठी (स्थिरांक V3GraphEdge& edge : ap->outEdges()) {
        जर (!m_edgeFuncp(&edge)) सुरूठेवा;

        अचिन्हित childCost;
        जर (pathExistsInternal(edge.शीर्ष(), bp, &childCost)) foundPath = सत्य;
        जर (costp) *costp += childCost;
    }

    परत foundPath;
}

तार्किक GraphPathChecker::pathExistsFrom(स्थिरांक V3GraphVertex* fromp, स्थिरांक V3GraphVertex* शीर्ष) {
    incGeneration();
    परत pathExistsInternal(fromp, शीर्ष);
}

तार्किक GraphPathChecker::isTransitiveEdge(स्थिरांक V3GraphEdge* edgep) {
    स्थिरांक V3GraphVertex* fromp = edgep->fromp();
    स्थिरांक V3GraphVertex* शीर्ष = edgep->शीर्ष();
    incGeneration();
    साठी (स्थिरांक V3GraphEdge& fromOut : fromp->outEdges()) {
        जर (&fromOut == edgep) सुरूठेवा;
        जर (pathExistsInternal(fromOut.शीर्ष(), शीर्ष)) परत सत्य;
    }
    परत असत्य;
}
