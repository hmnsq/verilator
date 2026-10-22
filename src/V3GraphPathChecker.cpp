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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3GraphPathChecker.h"

#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3GraphStream.h"

//######################################################################
// GraphPCNode

ഘടന GraphPCNode final {
    // User data for each node in GraphPathChecker.
    //
    // Like the LogicMTasks's, store before and after CPs for the nodes in
    // the GraphPathChecker graph.
    //
    // Unlike the LogicMTasks's, we have no cost info for the generic graph
    // accepted by GraphPathChecker, so assume each node has unit cost.
    std::array<uint32_t, GraphWay::അക്കം_WAYS> m_cp = {};

    // Detect if we've seen this node before in a given recursive
    // operation. We'll use this in pathExistsInternal() to avoid checking
    // the same node twice, and again in updateHalfCriticalPath() to assert
    // there are no cycles.
    uint64_t m_seenAtGeneration = 0;

    // CONSTRUCTORS
    GraphPCNode() = സ്ഥിരസ്ഥിതി;
    ~GraphPCNode() = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// GraphPathChecker implementation

template <GraphWay::en N_Way>
ശൂന്യം GraphPathChecker::initHalfCriticalPaths(തർക്കം checkOnly) {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay rev = way.invert();
    GraphStreamUnordered order(m_graphp, way);
    ആകുമ്പോൾ (സ്ഥിരം V3GraphVertex* സ്ഥിരം vertexp = order.nextp()) {
        unsigned critPathCost = 0;
        വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : vertexp->edges<rev>()) {
            എങ്കിൽ (!m_edgeFuncp(&edge)) തുടരുക;

            സ്ഥിരം V3GraphVertex* wrelativep = edge.furtherp<rev>();
            സ്ഥിരം GraphPCNode* സ്ഥിരം wrelUserp = static_cast<GraphPCNode*>(wrelativep->userp());
            critPathCost = std::max(critPathCost, wrelUserp->m_cp[way] + 1);
        }

        GraphPCNode* സ്ഥിരം ourUserp = static_cast<GraphPCNode*>(vertexp->userp());
        എങ്കിൽ (checkOnly) {
            UASSERT_OBJ(ourUserp->m_cp[way] == critPathCost, vertexp,
                        "Validation of critical paths failed");
        } അല്ലെങ്കിൽ {
            ourUserp->m_cp[way] = critPathCost;
        }
    }
}

template ശൂന്യം GraphPathChecker::initHalfCriticalPaths<GraphWay::FORWARD>(തർക്കം);
template ശൂന്യം GraphPathChecker::initHalfCriticalPaths<GraphWay::REVERSE>(തർക്കം);

GraphPathChecker::GraphPathChecker(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
    : GraphAlg<V3Graph>{graphp, edgeFuncp} {
    വേണ്ടി (V3GraphVertex& vtx : graphp->vertices()) {
        // Setup tracking structure for each node.  If delete a vertex
        // there would be a leak, but ok as accept only const V3Graph*'s.
        vtx.userp(പുതിയ GraphPCNode);
    }
    // Init critical paths in userp() for each vertex
    initHalfCriticalPaths<GraphWay::FORWARD>(അസത്യം);
    initHalfCriticalPaths<GraphWay::REVERSE>(അസത്യം);
}

GraphPathChecker::~GraphPathChecker() {
    // Free every GraphPCNode
    വേണ്ടി (V3GraphVertex& vtx : m_graphp->vertices()) {
        സ്ഥിരം GraphPCNode* സ്ഥിരം nodep = static_cast<GraphPCNode*>(vtx.userp());
        VL_DO_DANGLING(delete nodep, nodep);
        vtx.userp(nullptr);
    }
}

തർക്കം GraphPathChecker::pathExistsInternal(സ്ഥിരം V3GraphVertex* ap, സ്ഥിരം V3GraphVertex* bp,
                                          unsigned* costp) {
    GraphPCNode* സ്ഥിരം auserp = static_cast<GraphPCNode*>(ap->userp());
    സ്ഥിരം GraphPCNode* സ്ഥിരം buserp = static_cast<GraphPCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    എങ്കിൽ (auserp->m_seenAtGeneration == m_generation) {
        എങ്കിൽ (costp) *costp = 0;
        തിരികെ അസത്യം;
    }
    auserp->m_seenAtGeneration = m_generation;

    എങ്കിൽ (costp) *costp = 1;  // count 'a' toward the search cost

    എങ്കിൽ (ap == bp) തിരികെ സത്യം;

    // Rule out an a->b path based on their CPs
    എങ്കിൽ (auserp->m_cp[GraphWay::REVERSE] < buserp->m_cp[GraphWay::REVERSE] + 1) തിരികെ അസത്യം;
    എങ്കിൽ (buserp->m_cp[GraphWay::FORWARD] < auserp->m_cp[GraphWay::FORWARD] + 1) തിരികെ അസത്യം;

    // Slow path; visit some extended family
    തർക്കം foundPath = അസത്യം;
    വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : ap->outEdges()) {
        എങ്കിൽ (!m_edgeFuncp(&edge)) തുടരുക;

        unsigned childCost;
        എങ്കിൽ (pathExistsInternal(edge.മുകൾ(), bp, &childCost)) foundPath = സത്യം;
        എങ്കിൽ (costp) *costp += childCost;
    }

    തിരികെ foundPath;
}

തർക്കം GraphPathChecker::pathExistsFrom(സ്ഥിരം V3GraphVertex* fromp, സ്ഥിരം V3GraphVertex* മുകൾ) {
    incGeneration();
    തിരികെ pathExistsInternal(fromp, മുകൾ);
}

തർക്കം GraphPathChecker::isTransitiveEdge(സ്ഥിരം V3GraphEdge* edgep) {
    സ്ഥിരം V3GraphVertex* fromp = edgep->fromp();
    സ്ഥിരം V3GraphVertex* മുകൾ = edgep->മുകൾ();
    incGeneration();
    വേണ്ടി (സ്ഥിരം V3GraphEdge& fromOut : fromp->outEdges()) {
        എങ്കിൽ (&fromOut == edgep) തുടരുക;
        എങ്കിൽ (pathExistsInternal(fromOut.മുകൾ(), മുകൾ)) തിരികെ സത്യം;
    }
    തിരികെ അസത്യം;
}
