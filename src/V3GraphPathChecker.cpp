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

#घोषणा VL_MT_DISABLED_CODE_UNIT 1

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3GraphPathChecker.h"

#समावेश "V3Global.h"
#समावेश "V3GraphStream.h"

//######################################################################
// GraphPCNode

काष्ठा GraphPCNode final {
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
    GraphPCNode() = शेष;
    ~GraphPCNode() = शेष;
};

//######################################################################
// GraphPathChecker implementation

ढाँचा <GraphWay::en N_Way>
व्योम GraphPathChecker::initHalfCriticalPaths(bool checkOnly) {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay rev = way.invert();
    GraphStreamUnordered order(m_graphp, way);
    जबतक (स्थिर V3GraphVertex* स्थिर vertexp = order.nextp()) {
        अचिन्हित critPathCost = 0;
        क्रम (स्थिर V3GraphEdge& edge : vertexp->edges<rev>()) {
            अगर (!m_edgeFuncp(&edge)) जारी;

            स्थिर V3GraphVertex* wrelativep = edge.furtherp<rev>();
            स्थिर GraphPCNode* स्थिर wrelUserp = static_cast<GraphPCNode*>(wrelativep->userp());
            critPathCost = std::max(critPathCost, wrelUserp->m_cp[way] + 1);
        }

        GraphPCNode* स्थिर ourUserp = static_cast<GraphPCNode*>(vertexp->userp());
        अगर (checkOnly) {
            UASSERT_OBJ(ourUserp->m_cp[way] == critPathCost, vertexp,
                        "Validation of critical paths failed");
        } अन्यथा {
            ourUserp->m_cp[way] = critPathCost;
        }
    }
}

ढाँचा व्योम GraphPathChecker::initHalfCriticalPaths<GraphWay::FORWARD>(bool);
ढाँचा व्योम GraphPathChecker::initHalfCriticalPaths<GraphWay::REVERSE>(bool);

GraphPathChecker::GraphPathChecker(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
    : GraphAlg<V3Graph>{graphp, edgeFuncp} {
    क्रम (V3GraphVertex& vtx : graphp->vertices()) {
        // Setup tracking structure for each node.  If delete a vertex
        // there would be a leak, but ok as accept only const V3Graph*'s.
        vtx.userp(नया GraphPCNode);
    }
    // Init critical paths in userp() for each vertex
    initHalfCriticalPaths<GraphWay::FORWARD>(false);
    initHalfCriticalPaths<GraphWay::REVERSE>(false);
}

GraphPathChecker::~GraphPathChecker() {
    // Free every GraphPCNode
    क्रम (V3GraphVertex& vtx : m_graphp->vertices()) {
        स्थिर GraphPCNode* स्थिर nodep = static_cast<GraphPCNode*>(vtx.userp());
        VL_DO_DANGLING(मिटाओ nodep, nodep);
        vtx.userp(nullptr);
    }
}

bool GraphPathChecker::pathExistsInternal(स्थिर V3GraphVertex* ap, स्थिर V3GraphVertex* bp,
                                          अचिन्हित* costp) {
    GraphPCNode* स्थिर auserp = static_cast<GraphPCNode*>(ap->userp());
    स्थिर GraphPCNode* स्थिर buserp = static_cast<GraphPCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    अगर (auserp->m_seenAtGeneration == m_generation) {
        अगर (costp) *costp = 0;
        वापस false;
    }
    auserp->m_seenAtGeneration = m_generation;

    अगर (costp) *costp = 1;  // count 'a' toward the search cost

    अगर (ap == bp) वापस true;

    // Rule out an a->b path based on their CPs
    अगर (auserp->m_cp[GraphWay::REVERSE] < buserp->m_cp[GraphWay::REVERSE] + 1) वापस false;
    अगर (buserp->m_cp[GraphWay::FORWARD] < auserp->m_cp[GraphWay::FORWARD] + 1) वापस false;

    // Slow path; visit some extended family
    bool foundPath = false;
    क्रम (स्थिर V3GraphEdge& edge : ap->outEdges()) {
        अगर (!m_edgeFuncp(&edge)) जारी;

        अचिन्हित childCost;
        अगर (pathExistsInternal(edge.शीर्ष(), bp, &childCost)) foundPath = true;
        अगर (costp) *costp += childCost;
    }

    वापस foundPath;
}

bool GraphPathChecker::pathExistsFrom(स्थिर V3GraphVertex* fromp, स्थिर V3GraphVertex* शीर्ष) {
    incGeneration();
    वापस pathExistsInternal(fromp, शीर्ष);
}

bool GraphPathChecker::isTransitiveEdge(स्थिर V3GraphEdge* edgep) {
    स्थिर V3GraphVertex* fromp = edgep->fromp();
    स्थिर V3GraphVertex* शीर्ष = edgep->शीर्ष();
    incGeneration();
    क्रम (स्थिर V3GraphEdge& fromOut : fromp->outEdges()) {
        अगर (&fromOut == edgep) जारी;
        अगर (pathExistsInternal(fromOut.शीर्ष(), शीर्ष)) वापस true;
    }
    वापस false;
}
