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

#నిర్వచించు VL_MT_DISABLED_సంకేతం_UNIT 1

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3GraphPathChecker.h"

#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3GraphStream.h"

//######################################################################
// GraphPCNode

నిర్మాణం లేఖాచిత్రం_PCNode final {
    // User data for each node in GraphPathChecker.
    //
    // Like the LogicMTasks's, store before and after CPs for the nodes in
    // the GraphPathChecker graph.
    //
    // Unlike the LogicMTasks's, we have no cost info for the generic graph
    // accepted by GraphPathChecker, so assume each node has unit cost.
    std::array<uint32_t, లేఖాచిత్రం_Way::అంకె_WAYS> m_cp = {};

    // Detect if we've seen this node before in a given recursive
    // operation. We'll use this in pathExistsInternal() to avoid checking
    // the same node twice, and again in updateHalfCriticalPath() to assert
    // there are no cycles.
    uint64_t m_చూసిన_At_Generation = 0;

    // CONSTRUCTORS
    లేఖాచిత్రం_PCNode() = అప్రమేయ;
    ~లేఖాచిత్రం_PCNode() = అప్రమేయ;
};

//######################################################################
// GraphPathChecker implementation

మూస <లేఖాచిత్రం_Way::en N_Way>
శూన్య లేఖాచిత్రం_మార్గం_Checker::initHalfCriticalPaths(తార్కిక checkOnly) {
    constexpr లేఖాచిత్రం_Way way{N_Way};
    constexpr లేఖాచిత్రం_Way rev = way.invert();
    లేఖాచిత్రం_ప్రవాహం_Unordered క్రమం(m_graphp, way);
    అయితే (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vertexp = క్రమం.nextp()) {
        గుర్తులేని crit_మార్గం_ఖర్చు = 0;
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : vertexp->అంచులు<rev>()) {
            ఒకవేళ (!m_అంచు_Funcp(&అంచు)) కొనసాగు;

            స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* wrelativep = అంచు.furtherp<rev>();
            స్థిరరాశి లేఖాచిత్రం_PCNode* స్థిరరాశి wrelUserp = static_cast<లేఖాచిత్రం_PCNode*>(wrelativep->userp());
            crit_మార్గం_ఖర్చు = std::max(crit_మార్గం_ఖర్చు, wrelUserp->m_cp[way] + 1);
        }

        లేఖాచిత్రం_PCNode* స్థిరరాశి ourUserp = static_cast<లేఖాచిత్రం_PCNode*>(vertexp->userp());
        ఒకవేళ (checkOnly) {
            UASSERT_OBJ(ourUserp->m_cp[way] == crit_మార్గం_ఖర్చు, vertexp,
                        "Validation of critical paths failed");
        } లేకపోతే {
            ourUserp->m_cp[way] = crit_మార్గం_ఖర్చు;
        }
    }
}

మూస శూన్య లేఖాచిత్రం_మార్గం_Checker::initHalfCriticalPaths<లేఖాచిత్రం_Way::FORWARD>(తార్కిక);
మూస శూన్య లేఖాచిత్రం_మార్గం_Checker::initHalfCriticalPaths<లేఖాచిత్రం_Way::తిరగేయి_2>(తార్కిక);

లేఖాచిత్రం_మార్గం_Checker::లేఖాచిత్రం_మార్గం_Checker(V3_లేఖాచిత్రం* graphp, V3_అంచు_Func_P అంచు_Funcp)
    : లేఖాచిత్రం_Alg<V3_లేఖాచిత్రం>{graphp, అంచు_Funcp} {
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : graphp->vertices()) {
        // Setup tracking structure for each node.  If delete a vertex
        // there would be a leak, but ok as accept only const V3Graph*'s.
        vtx.userp(కొత్త లేఖాచిత్రం_PCNode);
    }
    // Init critical paths in userp() for each vertex
    initHalfCriticalPaths<లేఖాచిత్రం_Way::FORWARD>(అబద్ధం);
    initHalfCriticalPaths<లేఖాచిత్రం_Way::తిరగేయి_2>(అబద్ధం);
}

లేఖాచిత్రం_మార్గం_Checker::~లేఖాచిత్రం_మార్గం_Checker() {
    // Free every GraphPCNode
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_graphp->vertices()) {
        స్థిరరాశి లేఖాచిత్రం_PCNode* స్థిరరాశి nodep = static_cast<లేఖాచిత్రం_PCNode*>(vtx.userp());
        VL_DO_DANGLING(తొలగించు nodep, nodep);
        vtx.userp(శూన్యసూచిక);
    }
}

తార్కిక లేఖాచిత్రం_మార్గం_Checker::మార్గం_Exists_అంతర్గత(స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* ap, స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* bp,
                                          గుర్తులేని* costp) {
    లేఖాచిత్రం_PCNode* స్థిరరాశి auserp = static_cast<లేఖాచిత్రం_PCNode*>(ap->userp());
    స్థిరరాశి లేఖాచిత్రం_PCNode* స్థిరరాశి buserp = static_cast<లేఖాచిత్రం_PCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    ఒకవేళ (auserp->m_చూసిన_At_Generation == m_generation) {
        ఒకవేళ (costp) *costp = 0;
        తిరిగి అబద్ధం;
    }
    auserp->m_చూసిన_At_Generation = m_generation;

    ఒకవేళ (costp) *costp = 1;  // count 'a' toward the search cost

    ఒకవేళ (ap == bp) తిరిగి నిజం;

    // Rule out an a->b path based on their CPs
    ఒకవేళ (auserp->m_cp[లేఖాచిత్రం_Way::తిరగేయి_2] < buserp->m_cp[లేఖాచిత్రం_Way::తిరగేయి_2] + 1) తిరిగి అబద్ధం;
    ఒకవేళ (buserp->m_cp[లేఖాచిత్రం_Way::FORWARD] < auserp->m_cp[లేఖాచిత్రం_Way::FORWARD] + 1) తిరిగి అబద్ధం;

    // Slow path; visit some extended family
    తార్కిక దొరికింది_మార్గం = అబద్ధం;
    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : ap->బహిర్గతం_అంచులు()) {
        ఒకవేళ (!m_అంచు_Funcp(&అంచు)) కొనసాగు;

        గుర్తులేని బిడ్డ_ఖర్చు;
        ఒకవేళ (మార్గం_Exists_అంతర్గత(అంచు.శిఖరం(), bp, &బిడ్డ_ఖర్చు)) దొరికింది_మార్గం = నిజం;
        ఒకవేళ (costp) *costp += బిడ్డ_ఖర్చు;
    }

    తిరిగి దొరికింది_మార్గం;
}

తార్కిక లేఖాచిత్రం_మార్గం_Checker::మార్గం_Exists_From(స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* fromp, స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* శిఖరం) {
    incGeneration();
    తిరిగి మార్గం_Exists_అంతర్గత(fromp, శిఖరం);
}

తార్కిక లేఖాచిత్రం_మార్గం_Checker::is_సంక్రమణ_అంచు(స్థిరరాశి V3_లేఖాచిత్రం_అంచు* edgep) {
    స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* fromp = edgep->fromp();
    స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* శిఖరం = edgep->శిఖరం();
    incGeneration();
    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& from_బహిర్గతం : fromp->బహిర్గతం_అంచులు()) {
        ఒకవేళ (&from_బహిర్గతం == edgep) కొనసాగు;
        ఒకవేళ (మార్గం_Exists_అంతర్గత(from_బహిర్గతం.శిఖరం(), శిఖరం)) తిరిగి నిజం;
    }
    తిరిగి అబద్ధం;
}
