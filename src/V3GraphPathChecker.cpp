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
    std::array<uint32_t, GraphWay::अङ्क_WAYS> m_cp = {};

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
    जबसम्म (अचर V3GraphVertex* अचर vertexp = order.nextp()) {
        अचिन्हित critPathCost = 0;
        लागि (अचर V3GraphEdge& edge : vertexp->edges<rev>()) {
            यदि (!m_edgeFuncp(&edge)) जारी;

            अचर V3GraphVertex* wrelativep = edge.furtherp<rev>();
            अचर GraphPCNode* अचर wrelUserp = static_cast<GraphPCNode*>(wrelativep->userp());
            critPathCost = std::max(critPathCost, wrelUserp->m_cp[way] + 1);
        }

        GraphPCNode* अचर ourUserp = static_cast<GraphPCNode*>(vertexp->userp());
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
    लागि (V3GraphVertex& vtx : graphp->vertices()) {
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
    लागि (V3GraphVertex& vtx : m_graphp->vertices()) {
        अचर GraphPCNode* अचर nodep = static_cast<GraphPCNode*>(vtx.userp());
        VL_DO_DANGLING(delete nodep, nodep);
        vtx.userp(nullptr);
    }
}

तार्किक GraphPathChecker::pathExistsInternal(अचर V3GraphVertex* ap, अचर V3GraphVertex* bp,
                                          अचिन्हित* costp) {
    GraphPCNode* अचर auserp = static_cast<GraphPCNode*>(ap->userp());
    अचर GraphPCNode* अचर buserp = static_cast<GraphPCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    यदि (auserp->m_seenAtGeneration == m_generation) {
        यदि (costp) *costp = 0;
        फर्काऊ असत्य;
    }
    auserp->m_seenAtGeneration = m_generation;

    यदि (costp) *costp = 1;  // count 'a' toward the search cost

    यदि (ap == bp) फर्काऊ सत्य;

    // Rule out an a->b path based on their CPs
    यदि (auserp->m_cp[GraphWay::REVERSE] < buserp->m_cp[GraphWay::REVERSE] + 1) फर्काऊ असत्य;
    यदि (buserp->m_cp[GraphWay::FORWARD] < auserp->m_cp[GraphWay::FORWARD] + 1) फर्काऊ असत्य;

    // Slow path; visit some extended family
    तार्किक foundPath = असत्य;
    लागि (अचर V3GraphEdge& edge : ap->outEdges()) {
        यदि (!m_edgeFuncp(&edge)) जारी;

        अचिन्हित childCost;
        यदि (pathExistsInternal(edge.शीर्ष(), bp, &childCost)) foundPath = सत्य;
        यदि (costp) *costp += childCost;
    }

    फर्काऊ foundPath;
}

तार्किक GraphPathChecker::pathExistsFrom(अचर V3GraphVertex* fromp, अचर V3GraphVertex* शीर्ष) {
    incGeneration();
    फर्काऊ pathExistsInternal(fromp, शीर्ष);
}

तार्किक GraphPathChecker::isTransitiveEdge(अचर V3GraphEdge* edgep) {
    अचर V3GraphVertex* fromp = edgep->fromp();
    अचर V3GraphVertex* शीर्ष = edgep->शीर्ष();
    incGeneration();
    लागि (अचर V3GraphEdge& fromOut : fromp->outEdges()) {
        यदि (&fromOut == edgep) जारी;
        यदि (pathExistsInternal(fromOut.शीर्ष(), शीर्ष)) फर्काऊ सत्य;
    }
    फर्काऊ असत्य;
}
