// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Graph acyclic algorithm
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

#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3Graph.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <list>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - acyclic
//      Break the minimal number of backward edges to make the graph acyclic

తరగతి లేఖాచిత్రం_Acyc_శీర్షం final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(లేఖాచిత్రం_Acyc_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
    // user() is used for various sub-algorithm pieces
    V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి m_origVertexp;  // Pointer to first vertex this represents
రక్షిత:
    మిత్ర తరగతి లేఖాచిత్రం_Acyc;
    V3ListLinks<లేఖాచిత్రం_Acyc_శీర్షం> m_links;  // List links to store instances of this class
    uint32_t m_storedRank = 0;  // Rank held until commit to edge placement
    తార్కిక m_onWorkList = అబద్ధం;  // True if already on list of work to do
    తార్కిక m_deleted = అబద్ధం;  // True if deleted

అంతరంగ:
    V3ListLinks<లేఖాచిత్రం_Acyc_శీర్షం>& links() { తిరిగి m_links; }

బహిరంగ:
    // List type to store instances of this class
    ఉపయోగించి List = V3List<లేఖాచిత్రం_Acyc_శీర్షం, &లేఖాచిత్రం_Acyc_శీర్షం::links>;

    లేఖాచిత్రం_Acyc_శీర్షం(V3_లేఖాచిత్రం* graphp, V3_లేఖాచిత్రం_శీర్షం* origVertexp)
        : V3_లేఖాచిత్రం_శీర్షం{graphp}
        , m_origVertexp{origVertexp} {}
    ~లేఖాచిత్రం_Acyc_శీర్షం() override = అప్రమేయ;
    V3_లేఖాచిత్రం_శీర్షం* origVertexp() స్థిరరాశి { తిరిగి m_origVertexp; }
    శూన్య సమితి_Delete() { m_deleted = నిజం; }
    తార్కిక isDelete() స్థిరరాశి { తిరిగి m_deleted; }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string name() స్థిరరాశి override { తిరిగి m_origVertexp->name(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string dotColor() స్థిరరాశి override { తిరిగి m_origVertexp->dotColor(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    File_పంక్తి* fileline() స్థిరరాశి override { తిరిగి m_origVertexp->fileline(); }
};

//--------------------------------------------------------------------

తరగతి లేఖాచిత్రం_Acyc_అంచు final : బహిరంగ V3_లేఖాచిత్రం_అంచు {
    VL_RTTI_IMPL(లేఖాచిత్రం_Acyc_అంచు, V3_లేఖాచిత్రం_అంచు)
    // userp() is always used to point to the head original graph edge
అంతరంగ:
    ఉపయోగించి Orig_అంచు_List = std::list<V3_లేఖాచిత్రం_అంచు*>;  // List of orig edges, see also GraphAcyc's decl
    V3_లేఖాచిత్రం_అంచు* origEdgep() స్థిరరాశి {
        స్థిరరాశి Orig_అంచు_List* స్థిరరాశి oEListp = static_cast<Orig_అంచు_List*>(userp());
        UASSERT(oEListp, "No original edge associated with acyc edge " << ఇది);
        తిరిగి (oEListp->front());
    }

బహిరంగ:
    లేఖాచిత్రం_Acyc_అంచు(V3_లేఖాచిత్రం* graphp, V3_లేఖాచిత్రం_శీర్షం* fromp, V3_లేఖాచిత్రం_శీర్షం* శిఖరం, పూర్ణాంకం బరువు_2,
                  తార్కిక cutable = అబద్ధం)
        : V3_లేఖాచిత్రం_అంచు{graphp, fromp, శిఖరం, బరువు_2, cutable} {}
    ~లేఖాచిత్రం_Acyc_అంచు() override = అప్రమేయ;
    // yellow=we might still cut it, else oldEdge: yellowGreen=made uncutable, red=uncutable
    string dotColor() స్థిరరాశి override { తిరిగి (cutable() ? "yellow" : origEdgep()->dotColor()); }
};

//--------------------------------------------------------------------

నిర్మాణం లేఖాచిత్రం_Acyc_అంచు_Cmp final {
    తార్కిక క్రియాకారకం()(స్థిరరాశి V3_లేఖాచిత్రం_అంచు* lhsp, స్థిరరాశి V3_లేఖాచిత్రం_అంచు* rhsp) స్థిరరాశి {
        ఒకవేళ (lhsp->బరువు_2() > rhsp->బరువు_2()) తిరిగి నిజం;  // LHS goes first
        ఒకవేళ (lhsp->బరువు_2() < rhsp->బరువు_2()) తిరిగి అబద్ధం;  // RHS goes first
        తిరిగి అబద్ధం;
    }
};

//--------------------------------------------------------------------

// CLASSES
తరగతి లేఖాచిత్రం_Acyc final {
    ఉపయోగించి Orig_అంచు_List
        = std::list<V3_లేఖాచిత్రం_అంచు*>;  // List of orig edges, see also GraphAcycEdge's decl
    // GRAPH USERS
    //  origGraph
    //    GraphVertex::user()   GraphAycVerted* New graph node
    //  m_breakGraph
    //    GraphEdge::user()     OrigEdgeList*   Old graph edges
    //    GraphVertex::user     bool            Detection of loops in simplifyDupIterate
    // MEMBERS
    V3_లేఖాచిత్రం* స్థిరరాశి m_origGraphp;  // Original graph
    V3_లేఖాచిత్రం m_break_లేఖాచిత్రం;  // Graph with only breakable edges represented
    లేఖాచిత్రం_Acyc_శీర్షం::List m_work;  // List of vertices with optimization work left
    std::సదిశ<Orig_అంచు_List*> m_orig_అంచు_Delp;  // List of deletions to do when done
    స్థిరరాశి V3_అంచు_Func_P
        m_orig_అంచు_Funcp;  // Function that says we follow this edge (in original graph)
    uint32_t m_ఉంచు_అడుగు = 0;  // Number that user() must be equal to to indicate processing

    // METHODS
    శూన్య build_లేఖాచిత్రం(V3_లేఖాచిత్రం* origGraphp);
    శూన్య build_లేఖాచిత్రం_Iterate(V3_లేఖాచిత్రం_శీర్షం* overtexp, లేఖాచిత్రం_Acyc_శీర్షం* avertexp);
    శూన్య simplify(తార్కిక allowCut);
    శూన్య simplifyNone(లేఖాచిత్రం_Acyc_శీర్షం* avertexp);
    శూన్య simplifyOne(లేఖాచిత్రం_Acyc_శీర్షం* avertexp);
    శూన్య simplify_బహిర్గతం(లేఖాచిత్రం_Acyc_శీర్షం* avertexp);
    శూన్య simplifyDup(లేఖాచిత్రం_Acyc_శీర్షం* avertexp);
    శూన్య cutBasic(లేఖాచిత్రం_Acyc_శీర్షం* avertexp);
    శూన్య cutBackward(లేఖాచిత్రం_Acyc_శీర్షం* avertexp);
    శూన్య deleteMarked();
    శూన్య ఉంచు_2();
    శూన్య ఉంచు_Try_అంచు(V3_లేఖాచిత్రం_అంచు* edgep);
    తార్కిక ఉంచు_Iterate(లేఖాచిత్రం_Acyc_శీర్షం* vertexp, uint32_t currentRank);

    తార్కిక orig_Follow_అంచు(V3_లేఖాచిత్రం_అంచు* edgep) {
        తిరిగి (edgep->బరువు_2() && (m_orig_అంచు_Funcp)(edgep));
    }
    శూన్య అంచు_From_అంచు(V3_లేఖాచిత్రం_అంచు* oldedgep, V3_లేఖాచిత్రం_శీర్షం* fromp, V3_లేఖాచిత్రం_శీర్షం* శిఖరం) {
        // Make new breakGraph edge, with old edge as a template
        లేఖాచిత్రం_Acyc_అంచు* స్థిరరాశి newEdgep = కొత్త లేఖాచిత్రం_Acyc_అంచు{&m_break_లేఖాచిత్రం, fromp, శిఖరం,
                                                          oldedgep->బరువు_2(), oldedgep->cutable()};
        newEdgep->userp(oldedgep->userp());  // Keep pointer to OrigEdgeList
    }
    శూన్య కూడు_Orig_Edgep(V3_లేఖాచిత్రం_అంచు* గమ్యం_Edgep, V3_లేఖాచిత్రం_అంచు* కూడు_Edgep) {
        // Add addEdge (or it's list) to list of edges that break edge represents
        // Note addEdge may already have a bunch of similar linked edge representations.  Yuk.
        UASSERT(కూడు_Edgep, "Adding nullptr");
        ఒకవేళ (!గమ్యం_Edgep->userp()) {
            Orig_అంచు_List* స్థిరరాశి oep = కొత్త Orig_అంచు_List;
            m_orig_అంచు_Delp.నెట్టు_వెనుక(oep);
            గమ్యం_Edgep->userp(oep);
        }
        Orig_అంచు_List* స్థిరరాశి oEListp = static_cast<Orig_అంచు_List*>(గమ్యం_Edgep->userp());
        ఒకవేళ (Orig_అంచు_List* స్థిరరాశి కూడు_Listp = static_cast<Orig_అంచు_List*>(కూడు_Edgep->userp())) {
            కొరకు (స్థిరరాశి స్వయంచాలక& itr : *కూడు_Listp) oEListp->నెట్టు_వెనుక(itr);
            కూడు_Listp->clear();  // Done with it
        } లేకపోతే {
            oEListp->నెట్టు_వెనుక(కూడు_Edgep);
        }
    }
    శూన్య cut_Orig_అంచు(V3_లేఖాచిత్రం_అంచు* breakEdgep, స్థిరరాశి అక్షరం* why) {
        // From the break edge, cut edges in original graph it represents
        UINFO(8, why << " CUT " << breakEdgep->fromp());
        breakEdgep->cut();
        స్థిరరాశి Orig_అంచు_List* స్థిరరాశి oEListp = static_cast<Orig_అంచు_List*>(breakEdgep->userp());
        ఒకవేళ (!oEListp) {
            v3fatalSrc("No original edge associated with cutting edge " << breakEdgep);
        }
        // The breakGraph edge may represent multiple real edges; cut them all
        కొరకు (స్థిరరాశి స్వయంచాలక& origEdgep : *oEListp) {
            origEdgep->cut();
            UINFO(8, "  " << why << "   " << origEdgep->fromp() << " ->" << origEdgep->శిఖరం());
        }
    }
    // Work Queue
    శూన్య work_నెట్టు(V3_లేఖాచిత్రం_శీర్షం* vertexp) {
        లేఖాచిత్రం_Acyc_శీర్షం* స్థిరరాశి avertexp = static_cast<లేఖాచిత్రం_Acyc_శీర్షం*>(vertexp);
        // Add vertex to list of nodes needing further optimization trials
        ఒకవేళ (!avertexp->m_onWorkList) {
            avertexp->m_onWorkList = నిజం;
            m_work.link_వెనుక(avertexp);
        }
    }
    లేఖాచిత్రం_Acyc_శీర్షం* workBeginp() { తిరిగి m_work.frontp(); }
    శూన్య work_తీయి() {
        లేఖాచిత్రం_Acyc_శీర్షం* స్థిరరాశి avertexp = workBeginp();
        avertexp->m_onWorkList = అబద్ధం;
        m_work.unlink(avertexp);
    }

బహిరంగ:
    // CONSTRUCTORS
    లేఖాచిత్రం_Acyc(V3_లేఖాచిత్రం* origGraphp, V3_అంచు_Func_P అంచు_Funcp)
        : m_origGraphp{origGraphp}
        , m_orig_అంచు_Funcp{అంచు_Funcp} {}
    ~లేఖాచిత్రం_Acyc() {
        కొరకు (Orig_అంచు_List* ip : m_orig_అంచు_Delp) తొలగించు ip;
        m_orig_అంచు_Delp.clear();
    }
    శూన్య ప్రధాన();
};

//--------------------------------------------------------------------

శూన్య లేఖాచిత్రం_Acyc::build_లేఖాచిత్రం(V3_లేఖాచిత్రం* origGraphp) {
    // Presumes the graph has been strongly ordered,
    // and thus there's a unique color if there are loops in this subgraph.

    // For each old node, make a new graph node for optimization
    origGraphp->userClearVertices();
    origGraphp->user_Clear_అంచులు();
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& overtex : origGraphp->vertices()) {
        ఒకవేళ (overtex.color()) {
            లేఖాచిత్రం_Acyc_శీర్షం* స్థిరరాశి avertexp = కొత్త లేఖాచిత్రం_Acyc_శీర్షం{&m_break_లేఖాచిత్రం, &overtex};
            overtex.userp(avertexp);  // Stash so can look up later
        }
    }

    // Build edges between logic vertices
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& overtex : origGraphp->vertices()) {
        ఒకవేళ (overtex.color()) {
            లేఖాచిత్రం_Acyc_శీర్షం* స్థిరరాశి avertexp = static_cast<లేఖాచిత్రం_Acyc_శీర్షం*>(overtex.userp());
            build_లేఖాచిత్రం_Iterate(&overtex, avertexp);
        }
    }
}

శూన్య లేఖాచిత్రం_Acyc::build_లేఖాచిత్రం_Iterate(V3_లేఖాచిత్రం_శీర్షం* overtexp, లేఖాచిత్రం_Acyc_శీర్షం* avertexp) {
    // Make new edges
    కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : overtexp->బహిర్గతం_అంచులు()) {
        ఒకవేళ (orig_Follow_అంచు(&అంచు)) {  // not cut
            స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* గమ్యం_Vertexp = అంచు.శిఖరం();
            ఒకవేళ (గమ్యం_Vertexp->color()) {
                లేఖాచిత్రం_Acyc_శీర్షం* స్థిరరాశి గమ్యం_AVertexp
                    = static_cast<లేఖాచిత్రం_Acyc_శీర్షం*>(గమ్యం_Vertexp->userp());
                // Replicate the old edge into the new graph
                // There may be multiple edges between same pairs of vertices
                V3_లేఖాచిత్రం_అంచు* breakEdgep = కొత్త లేఖాచిత్రం_Acyc_అంచు{&m_break_లేఖాచిత్రం, avertexp, గమ్యం_AVertexp,
                                                            అంచు.బరువు_2(), అంచు.cutable()};
                కూడు_Orig_Edgep(breakEdgep, &అంచు);  // So can find original edge
            }
        }
    }
}

శూన్య లేఖాచిత్రం_Acyc::simplify(తార్కిక allowCut) {
    // Add all nodes to list of work to do
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& శీర్షం : m_break_లేఖాచిత్రం.vertices()) work_నెట్టు(&శీర్షం);
    // Optimize till everything finished
    అయితే (లేఖాచిత్రం_Acyc_శీర్షం* vertexp = workBeginp()) {
        work_తీయి();
        simplifyNone(vertexp);
        simplifyOne(vertexp);
        simplify_బహిర్గతం(vertexp);
        simplifyDup(vertexp);
        ఒకవేళ (allowCut) {
            // The main algorithm works without these, though slower
            // So if changing the main algorithm, comment these out for a test run
            ఒకవేళ (v3Global.opt.fAcycSimp()) {
                cutBasic(vertexp);
                cutBackward(vertexp);
            }
        }
    }
    deleteMarked();
}

శూన్య లేఖాచిత్రం_Acyc::deleteMarked() {
    // Delete nodes marked for removal
    కొరకు (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp : m_break_లేఖాచిత్రం.vertices().unlinkable()) {
        లేఖాచిత్రం_Acyc_శీర్షం* స్థిరరాశి avertexp = static_cast<లేఖాచిత్రం_Acyc_శీర్షం*>(vtxp);
        ఒకవేళ (avertexp->isDelete()) {
            VL_DO_DANGLING(avertexp->unlinkDelete(&m_break_లేఖాచిత్రం), avertexp);
        }
    }
}

శూన్య లేఖాచిత్రం_Acyc::simplifyNone(లేఖాచిత్రం_Acyc_శీర్షం* avertexp) {
    // Don't need any vertices with no inputs, There's no way they can have a loop.
    // Likewise, vertices with no outputs
    ఒకవేళ (avertexp->isDelete()) తిరిగి;
    ఒకవేళ (avertexp->లోపల_ఖాళీలు() || avertexp->బహిర్గతం_ఖాళీలు()) {
        UINFO(9, "  SimplifyNoneRemove " << avertexp);
        avertexp->సమితి_Delete();  // Mark so we won't delete it twice
        // Remove edges
        అయితే (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep = avertexp->బహిర్గతం_అంచులు().frontp()) {
            work_నెట్టు(edgep->శిఖరం());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        అయితే (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep = avertexp->లోపల_అంచులు().frontp()) {
            work_నెట్టు(edgep->fromp());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
}

శూన్య లేఖాచిత్రం_Acyc::simplifyOne(లేఖాచిత్రం_Acyc_శీర్షం* avertexp) {
    // If a node has one input and one output, we can remove it and change the edges
    ఒకవేళ (avertexp->isDelete()) తిరిగి;
    ఒకవేళ (avertexp->లోపల_Size1() && avertexp->బహిర్గతం_Size1()) {
        V3_లేఖాచిత్రం_అంచు* స్థిరరాశి లోపల_Edgep = avertexp->లోపల_అంచులు().frontp();
        V3_లేఖాచిత్రం_అంచు* స్థిరరాశి బహిర్గతం_Edgep = avertexp->బహిర్గతం_అంచులు().frontp();
        V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి లోపల_Vertexp = లోపల_Edgep->fromp();
        V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి బహిర్గతం_Vertexp = బహిర్గతం_Edgep->శిఖరం();
        // The in and out may be the same node; we'll make a loop
        // The in OR out may be THIS node; we can't delete it then.
        ఒకవేళ (లోపల_Vertexp != avertexp && బహిర్గతం_Vertexp != avertexp) {
            UINFO(9, "  SimplifyOneRemove " << avertexp);
            avertexp->సమితి_Delete();  // Mark so we won't delete it twice
            // Make a new edge connecting the two vertices directly
            // If both are breakable, we pick the one with less weight, else it's arbitrary
            // We can forget about the origEdge list for the "non-selected" set of edges,
            // as we need to break only one set or the other set of edges, not both.
            // (This is why we must give preference to the cutable set.)
            V3_లేఖాచిత్రం_అంచు* స్థిరరాశి templateEdgep
                = ((లోపల_Edgep->cutable()
                    && (!బహిర్గతం_Edgep->cutable() || లోపల_Edgep->బరువు_2() < బహిర్గతం_Edgep->బరువు_2()))
                       ? లోపల_Edgep
                       : బహిర్గతం_Edgep);
            // cppcheck-suppress leakReturnValNotUsed
            అంచు_From_అంచు(templateEdgep, లోపల_Vertexp, బహిర్గతం_Vertexp);
            // Remove old edge
            VL_DO_DANGLING(లోపల_Edgep->unlinkDelete(), లోపల_Edgep);
            VL_DO_DANGLING(బహిర్గతం_Edgep->unlinkDelete(), బహిర్గతం_Edgep);
            VL_DANGLING(templateEdgep);
            work_నెట్టు(లోపల_Vertexp);
            work_నెట్టు(బహిర్గతం_Vertexp);
        }
    }
}

శూన్య లేఖాచిత్రం_Acyc::simplify_బహిర్గతం(లేఖాచిత్రం_Acyc_శీర్షం* avertexp) {
    // If a node has one output that's not cutable, all its inputs can be reassigned
    // to the next node in the list
    ఒకవేళ (avertexp->isDelete()) తిరిగి;
    ఒకవేళ (avertexp->బహిర్గతం_Size1()) {
        V3_లేఖాచిత్రం_అంచు* స్థిరరాశి బహిర్గతం_Edgep = avertexp->బహిర్గతం_అంచులు().frontp();
        ఒకవేళ (!బహిర్గతం_Edgep->cutable()) {
            V3_లేఖాచిత్రం_శీర్షం* బహిర్గతం_Vertexp = బహిర్గతం_Edgep->శిఖరం();
            UINFO(9, "  SimplifyOutRemove " << avertexp);
            avertexp->సమితి_Delete();  // Mark so we won't delete it twice
            కొరకు (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి లోపల_Edgep : avertexp->లోపల_అంచులు().unlinkable()) {
                V3_లేఖాచిత్రం_శీర్షం* లోపల_Vertexp = లోపల_Edgep->fromp();
                ఒకవేళ (లోపల_Vertexp == avertexp) {
                    ఒకవేళ (debug()) v3error("Non-cutable vertex=" << avertexp);  // LCOV_EXCL_LINE
                    v3error("Circular logic when ordering code (non-cutable edge loop)\n"
                            << m_origGraphp->reportLoops(  // calls OrderGraph::loopsVertexCb
                                   &V3_లేఖాచిత్రం_అంచు::followNotCutable, avertexp->origVertexp()));
                    // Things are unlikely to end well at this point,
                    // but we'll try something to get to further errors...
                    లోపల_Edgep->cutable(నిజం);
                    తిరిగి;
                }
                // Make a new edge connecting the two vertices directly
                // cppcheck-suppress leakReturnValNotUsed
                అంచు_From_అంచు(లోపల_Edgep, లోపల_Vertexp, బహిర్గతం_Vertexp);
                // Remove old edge
                VL_DO_DANGLING(లోపల_Edgep->unlinkDelete(), లోపల_Edgep);
                work_నెట్టు(లోపల_Vertexp);
            }
            VL_DO_DANGLING(బహిర్గతం_Edgep->unlinkDelete(), బహిర్గతం_Edgep);
            work_నెట్టు(బహిర్గతం_Vertexp);
        }
    }
}

శూన్య లేఖాచిత్రం_Acyc::simplifyDup(లేఖాచిత్రం_Acyc_శీర్షం* avertexp) {
    // Remove redundant edges
    ఒకవేళ (avertexp->isDelete()) తిరిగి;
    // Clear marks
    కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : avertexp->బహిర్గతం_అంచులు()) అంచు.శిఖరం()->userp(శూన్యసూచిక);
    // Mark edges and detect duplications
    కొరకు (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep : avertexp->బహిర్గతం_అంచులు().unlinkable()) {
        V3_లేఖాచిత్రం_శీర్షం* బహిర్గతం_Vertexp = edgep->శిఖరం();
        V3_లేఖాచిత్రం_అంచు* prevEdgep = static_cast<V3_లేఖాచిత్రం_అంచు*>(బహిర్గతం_Vertexp->userp());
        ఒకవేళ (prevEdgep) {
            ఒకవేళ (!prevEdgep->cutable()) {
                // !cutable duplicates prev !cutable: we can ignore it, redundant
                //  cutable duplicates prev !cutable: know it's not a relevant loop, ignore it
                UINFO(8, "    DelDupEdge " << avertexp << " -> " << edgep->శిఖరం());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            } లేకపోతే ఒకవేళ (!edgep->cutable()) {
                // !cutable duplicates prev  cutable: delete the earlier cutable
                UINFO(8, "    DelDupPrev " << avertexp << " -> " << prevEdgep->శిఖరం());
                VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                బహిర్గతం_Vertexp->userp(edgep);
            } లేకపోతే {
                //  cutable duplicates prev  cutable: combine weights
                UINFO(8, "    DelDupComb " << avertexp << " -> " << edgep->శిఖరం());
                prevEdgep->బరువు_2(prevEdgep->బరువు_2() + edgep->బరువు_2());
                కూడు_Orig_Edgep(prevEdgep, edgep);
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            work_నెట్టు(బహిర్గతం_Vertexp);
            work_నెట్టు(avertexp);
        } లేకపోతే {
            // No previous assignment
            బహిర్గతం_Vertexp->userp(edgep);
        }
    }
}

శూన్య లేఖాచిత్రం_Acyc::cutBasic(లేఖాచిత్రం_Acyc_శీర్షం* avertexp) {
    // Detect and cleanup any loops from node to itself
    ఒకవేళ (avertexp->isDelete()) తిరిగి;
    కొరకు (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep : avertexp->బహిర్గతం_అంచులు().unlinkable()) {
        ఒకవేళ (edgep->cutable() && edgep->శిఖరం() == avertexp) {
            cut_Orig_అంచు(edgep, "  Cut Basic");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_నెట్టు(avertexp);
        }
    }
}

శూన్య లేఖాచిత్రం_Acyc::cutBackward(లేఖాచిత్రం_Acyc_శీర్షం* avertexp) {
    // If a cutable edge is from A->B, and there's a non-cutable edge B->A, then must cut!
    ఒకవేళ (avertexp->isDelete()) తిరిగి;
    // Clear marks
    కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : avertexp->బహిర్గతం_అంచులు()) అంచు.శిఖరం()->user(అబద్ధం);
    కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : avertexp->లోపల_అంచులు()) {
        ఒకవేళ (!అంచు.cutable()) అంచు.fromp()->user(నిజం);
    }
    // Detect duplications
    కొరకు (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep : avertexp->బహిర్గతం_అంచులు().unlinkable()) {
        ఒకవేళ (edgep->cutable() && edgep->శిఖరం()->user()) {
            cut_Orig_అంచు(edgep, "  Cut A->B->A");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_నెట్టు(avertexp);
        }
    }
}

శూన్య లేఖాచిత్రం_Acyc::ఉంచు_2() {
    // Input is m_breakGraph with ranks already assigned on non-breakable edges

    // Make a list of all cutable edges in the graph
    పూర్ణాంకం అంకె_అంచులు = 0;
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& శీర్షం : m_break_లేఖాచిత్రం.vertices()) {
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : శీర్షం.బహిర్గతం_అంచులు()) {
            ఒకవేళ (అంచు.బరువు_2() && అంచు.cutable()) ++అంకె_అంచులు;
        }
    }
    UINFO(4, "    Cutable edges = " << అంకె_అంచులు);

    std::సదిశ<V3_లేఖాచిత్రం_అంచు*> అంచులు;  // List of all edges to be processed
    // Make the vector properly sized right off the bat -- faster than reallocating
    అంచులు.reserve(అంకె_అంచులు + 1);
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& శీర్షం : m_break_లేఖాచిత్రం.vertices()) {
        శీర్షం.user(0);  // Clear in prep of next step
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : శీర్షం.బహిర్గతం_అంచులు()) {
            ఒకవేళ (అంచు.బరువు_2() && అంచు.cutable()) అంచులు.నెట్టు_వెనుక(&అంచు);
        }
    }

    // Sort by weight, then by vertex (so that we completely process one vertex, when possible)
    stable_క్రమీకరించు(అంచులు.begin(), అంచులు.చివర(), లేఖాచిత్రం_Acyc_అంచు_Cmp());

    // Process each edge in weighted order
    m_ఉంచు_అడుగు = 10;
    కొరకు (V3_లేఖాచిత్రం_అంచు* edgep : అంచులు) ఉంచు_Try_అంచు(edgep);
}

శూన్య లేఖాచిత్రం_Acyc::ఉంచు_Try_అంచు(V3_లేఖాచిత్రం_అంచు* edgep) {
    // Try to make this edge uncutable
    m_ఉంచు_అడుగు++;
    UINFO(8, "    PlaceEdge s" << m_ఉంచు_అడుగు << " w" << edgep->బరువు_2() << " " << edgep->fromp());
    // Make the edge uncutable so we detect it in placement
    edgep->cutable(అబద్ధం);
    // Vertex::m_user begin: number indicates this edge was completed
    // Try to assign ranks, presuming this edge is in place
    // If we come across user()==placestep, we've detected a loop and must back out
    స్థిరరాశి తార్కిక loop
        = ఉంచు_Iterate(static_cast<లేఖాచిత్రం_Acyc_శీర్షం*>(edgep->శిఖరం()), edgep->fromp()->rank() + 1);
    ఒకవేళ (!loop) {
        // No loop, we can keep it as uncutable
        // Commit the new ranks we calculated
        // Just cleanup the list.  If this is slow, we can add another set of
        // user counters to avoid cleaning up the list.
        అయితే (workBeginp()) work_తీయి();
    } లేకపోతే {
        // Adding this edge would cause a loop, kill it
        edgep->cutable(నిజం);  // So graph still looks pretty
        cut_Orig_అంచు(edgep, "  Cut loop");
        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        // Back out the ranks we calculated
        అయితే (లేఖాచిత్రం_Acyc_శీర్షం* vertexp = workBeginp()) {
            work_తీయి();
            vertexp->rank(vertexp->m_storedRank);
        }
    }
}

తార్కిక లేఖాచిత్రం_Acyc::ఉంచు_Iterate(లేఖాచిత్రం_Acyc_శీర్షం* vertexp, uint32_t currentRank) {
    // Assign rank to each unvisited node
    //   rank() is the "committed rank" of the graph known without loops
    // If larger rank is found, assign it and loop back through
    // If we hit a back node make a list of all loops
    ఒకవేళ (vertexp->rank() >= currentRank) తిరిగి అబద్ధం;  // Already processed it
    ఒకవేళ (vertexp->user() == m_ఉంచు_అడుగు) తిరిగి నిజం;  // Loop detected
    vertexp->user(m_ఉంచు_అడుగు);
    // Remember we're changing the rank of this node; might need to back out
    ఒకవేళ (!vertexp->m_onWorkList) {
        vertexp->m_storedRank = vertexp->rank();
        work_నెట్టు(vertexp);
    }
    vertexp->rank(currentRank);
    // Follow all edges and increase their ranks
    కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vertexp->బహిర్గతం_అంచులు()) {
        ఒకవేళ (అంచు.బరువు_2() && !అంచు.cutable()) {
            ఒకవేళ (ఉంచు_Iterate(static_cast<లేఖాచిత్రం_Acyc_శీర్షం*>(అంచు.శిఖరం()), currentRank + 1)) {
                // We don't need to reset user(); we'll use a different placeStep for the next edge
                తిరిగి నిజం;  // Loop detected
            }
        }
    }
    vertexp->user(0);
    తిరిగి అబద్ధం;
}

//----- Main algorithm entry point

శూన్య లేఖాచిత్రం_Acyc::ప్రధాన() {
    m_break_లేఖాచిత్రం.user_Clear_అంచులు();

    // Color based on possible loops
    m_origGraphp->stronglyConnected(m_orig_అంచు_Funcp);

    // Make a new graph with vertices that have only a single vertex
    // for each group of old vertices that are interconnected with unbreakable
    // edges (and thus can't represent loops - if we did the unbreakable
    // marking right, anyways)
    build_లేఖాచిత్రం(m_origGraphp);
    ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) m_break_లేఖాచిత్రం.dumpDotFilePrefixed("acyc_pre");

    // Perform simple optimizations before any cuttings
    simplify(అబద్ధం);
    ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 5) m_break_లేఖాచిత్రం.dumpDotFilePrefixed("acyc_simp");

    UINFO(4, " Cutting trivial loops");
    simplify(నిజం);
    ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) m_break_లేఖాచిత్రం.dumpDotFilePrefixed("acyc_mid");

    UINFO(4, " Ranking");
    m_break_లేఖాచిత్రం.rank(&V3_లేఖాచిత్రం_అంచు::followNotCutable);
    ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) m_break_లేఖాచిత్రం.dumpDotFilePrefixed("acyc_rank");

    UINFO(4, " Placement");
    ఉంచు_2();
    ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) m_break_లేఖాచిత్రం.dumpDotFilePrefixed("acyc_place");

    UINFO(4, " Final Ranking");
    // Only needed to assert there are no loops in completed graph
    m_break_లేఖాచిత్రం.rank(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);
    ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) m_break_లేఖాచిత్రం.dumpDotFilePrefixed("acyc_done");
}

శూన్య V3_లేఖాచిత్రం::acyclic(V3_అంచు_Func_P అంచు_Funcp) {
    UINFO(4, "Acyclic");
    లేఖాచిత్రం_Acyc acyc{ఇది, అంచు_Funcp};
    acyc.ప్రధాన();
    UINFO(4, "Acyclic done");
}
