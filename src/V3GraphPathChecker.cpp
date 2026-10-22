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

#परिभाषय VL_MT_DISABLED_संहिता_UNIT 1

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
    std::array<uint32_t, GraphWay::अङ्कः_WAYS> m_cp = {};

    // Detect if we've seen this node before in a given recursive
    // operation. We'll use this in pathExistsInternal() to avoid checking
    // the same node twice, and again in updateHalfCriticalPath() to assert
    // there are no cycles.
    uint64_t m_आलोकितम्_At_Generation = 0;

    // CONSTRUCTORS
    GraphPCNode() = स्वभाव;
    ~GraphPCNode() = स्वभाव;
};

//######################################################################
// GraphPathChecker implementation

प्रतिरूपम् <GraphWay::en N_Way>
शून्य Graph_मार्गः_Checker::initHalfCriticalPaths(तार्किक checkOnly) {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay rev = way.invert();
    Graph_प्रवाहः_Unordered क्रमः(m_graphp, way);
    यावत् (ध्रुव V3GraphVertex* ध्रुव vertexp = क्रमः.nextp()) {
        अचिह्नित crit_मार्गः_Cost = 0;
        प्रति (ध्रुव V3GraphEdge& edge : vertexp->धाराः<rev>()) {
            यदि (!m_edgeFuncp(&edge)) अनुवर्त;

            ध्रुव V3GraphVertex* wrelativep = edge.furtherp<rev>();
            ध्रुव GraphPCNode* ध्रुव wrelUserp = static_cast<GraphPCNode*>(wrelativep->userp());
            crit_मार्गः_Cost = std::max(crit_मार्गः_Cost, wrelUserp->m_cp[way] + 1);
        }

        GraphPCNode* ध्रुव ourUserp = static_cast<GraphPCNode*>(vertexp->userp());
        यदि (checkOnly) {
            UASSERT_OBJ(ourUserp->m_cp[way] == crit_मार्गः_Cost, vertexp,
                        "Validation of critical paths failed");
        } अन्यथा {
            ourUserp->m_cp[way] = crit_मार्गः_Cost;
        }
    }
}

प्रतिरूपम् शून्य Graph_मार्गः_Checker::initHalfCriticalPaths<GraphWay::FORWARD>(तार्किक);
प्रतिरूपम् शून्य Graph_मार्गः_Checker::initHalfCriticalPaths<GraphWay::REVERSE>(तार्किक);

Graph_मार्गः_Checker::Graph_मार्गः_Checker(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
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

Graph_मार्गः_Checker::~Graph_मार्गः_Checker() {
    // Free every GraphPCNode
    प्रति (V3GraphVertex& vtx : m_graphp->vertices()) {
        ध्रुव GraphPCNode* ध्रुव nodep = static_cast<GraphPCNode*>(vtx.userp());
        VL_DO_DANGLING(विलोपय nodep, nodep);
        vtx.userp(शून्यसूचकः);
    }
}

तार्किक Graph_मार्गः_Checker::मार्गः_Exists_आन्तरिकम्(ध्रुव V3GraphVertex* ap, ध्रुव V3GraphVertex* bp,
                                          अचिह्नित* costp) {
    GraphPCNode* ध्रुव auserp = static_cast<GraphPCNode*>(ap->userp());
    ध्रुव GraphPCNode* ध्रुव buserp = static_cast<GraphPCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    यदि (auserp->m_आलोकितम्_At_Generation == m_generation) {
        यदि (costp) *costp = 0;
        प्रत्यागम असत्य;
    }
    auserp->m_आलोकितम्_At_Generation = m_generation;

    यदि (costp) *costp = 1;  // count 'a' toward the search cost

    यदि (ap == bp) प्रत्यागम सत्य;

    // Rule out an a->b path based on their CPs
    यदि (auserp->m_cp[GraphWay::REVERSE] < buserp->m_cp[GraphWay::REVERSE] + 1) प्रत्यागम असत्य;
    यदि (buserp->m_cp[GraphWay::FORWARD] < auserp->m_cp[GraphWay::FORWARD] + 1) प्रत्यागम असत्य;

    // Slow path; visit some extended family
    तार्किक प्राप्तम्_मार्गः = असत्य;
    प्रति (ध्रुव V3GraphEdge& edge : ap->बहिर्गमनम्_धाराः()) {
        यदि (!m_edgeFuncp(&edge)) अनुवर्त;

        अचिह्नित childCost;
        यदि (मार्गः_Exists_आन्तरिकम्(edge.शिखरम्(), bp, &childCost)) प्राप्तम्_मार्गः = सत्य;
        यदि (costp) *costp += childCost;
    }

    प्रत्यागम प्राप्तम्_मार्गः;
}

तार्किक Graph_मार्गः_Checker::मार्गः_Exists_From(ध्रुव V3GraphVertex* fromp, ध्रुव V3GraphVertex* शिखरम्) {
    incGeneration();
    प्रत्यागम मार्गः_Exists_आन्तरिकम्(fromp, शिखरम्);
}

तार्किक Graph_मार्गः_Checker::is_संक्रामकम्_Edge(ध्रुव V3GraphEdge* edgep) {
    ध्रुव V3GraphVertex* fromp = edgep->fromp();
    ध्रुव V3GraphVertex* शिखरम् = edgep->शिखरम्();
    incGeneration();
    प्रति (ध्रुव V3GraphEdge& from_बहिर्गमनम् : fromp->बहिर्गमनम्_धाराः()) {
        यदि (&from_बहिर्गमनम् == edgep) अनुवर्त;
        यदि (मार्गः_Exists_आन्तरिकम्(from_बहिर्गमनम्.शिखरम्(), शिखरम्)) प्रत्यागम सत्य;
    }
    प्रत्यागम असत्य;
}
