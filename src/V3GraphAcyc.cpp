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

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_MT_DISABLED_ਕੋਡ_UNIT 1

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <list>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - acyclic
//      Break the minimal number of backward edges to make the graph acyclic

ਵਰਗ ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
    // user() is used for various sub-algorithm pieces
    V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ m_origVertexp;  // Pointer to first vertex this represents
ਸੁਰੱਖਿਅਤ:
    ਮਿੱਤਰ ਵਰਗ ਗ੍ਰਾਫ_Acyc;
    V3ListLinks<ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼> m_links;  // List links to store instances of this class
    uint32_t m_storedRank = 0;  // Rank held until commit to edge placement
    ਤਾਰਕਿਕ m_onWorkList = ਝੂਠ;  // True if already on list of work to do
    ਤਾਰਕਿਕ m_deleted = ਝੂਠ;  // True if deleted

ਨਿਜੀ:
    V3ListLinks<ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼>& links() { ਵਾਪਸ m_links; }

ਜਨਤਕ:
    // List type to store instances of this class
    ਵਰਤੋਂ List = V3List<ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼, &ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼::links>;

    ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* origVertexp)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp}
        , m_origVertexp{origVertexp} {}
    ~ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼() override = ਮੂਲ;
    V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* origVertexp() ਅਚਲ { ਵਾਪਸ m_origVertexp; }
    ਖਾਲੀ ਸੈੱਟ_Delete() { m_deleted = ਸੱਚ; }
    ਤਾਰਕਿਕ isDelete() ਅਚਲ { ਵਾਪਸ m_deleted; }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string name() ਅਚਲ override { ਵਾਪਸ m_origVertexp->name(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string dotColor() ਅਚਲ override { ਵਾਪਸ m_origVertexp->dotColor(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    File_ਸਤਰ* fileline() ਅਚਲ override { ਵਾਪਸ m_origVertexp->fileline(); }
};

//--------------------------------------------------------------------

ਵਰਗ ਗ੍ਰਾਫ_Acyc_ਕਿਨਾਰਾ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ {
    VL_RTTI_IMPL(ਗ੍ਰਾਫ_Acyc_ਕਿਨਾਰਾ, V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ)
    // userp() is always used to point to the head original graph edge
ਨਿਜੀ:
    ਵਰਤੋਂ Orig_ਕਿਨਾਰਾ_List = std::list<V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ*>;  // List of orig edges, see also GraphAcyc's decl
    V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* origEdgep() ਅਚਲ {
        ਅਚਲ Orig_ਕਿਨਾਰਾ_List* ਅਚਲ oEListp = static_cast<Orig_ਕਿਨਾਰਾ_List*>(userp());
        UASSERT(oEListp, "No original edge associated with acyc edge " << ਇਹ);
        ਵਾਪਸ (oEListp->front());
    }

ਜਨਤਕ:
    ਗ੍ਰਾਫ_Acyc_ਕਿਨਾਰਾ(V3_ਗ੍ਰਾਫ* graphp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ, ਪੂਰਨ ਭਾਰ_2,
                  ਤਾਰਕਿਕ cutable = ਝੂਠ)
        : V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{graphp, fromp, ਸਿਖਰ, ਭਾਰ_2, cutable} {}
    ~ਗ੍ਰਾਫ_Acyc_ਕਿਨਾਰਾ() override = ਮੂਲ;
    // yellow=we might still cut it, else oldEdge: yellowGreen=made uncutable, red=uncutable
    string dotColor() ਅਚਲ override { ਵਾਪਸ (cutable() ? "yellow" : origEdgep()->dotColor()); }
};

//--------------------------------------------------------------------

ਬਣਤਰ ਗ੍ਰਾਫ_Acyc_ਕਿਨਾਰਾ_Cmp final {
    ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* lhsp, ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* rhsp) ਅਚਲ {
        ਜੇ (lhsp->ਭਾਰ_2() > rhsp->ਭਾਰ_2()) ਵਾਪਸ ਸੱਚ;  // LHS goes first
        ਜੇ (lhsp->ਭਾਰ_2() < rhsp->ਭਾਰ_2()) ਵਾਪਸ ਝੂਠ;  // RHS goes first
        ਵਾਪਸ ਝੂਠ;
    }
};

//--------------------------------------------------------------------

// CLASSES
ਵਰਗ ਗ੍ਰਾਫ_Acyc final {
    ਵਰਤੋਂ Orig_ਕਿਨਾਰਾ_List
        = std::list<V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ*>;  // List of orig edges, see also GraphAcycEdge's decl
    // GRAPH USERS
    //  origGraph
    //    GraphVertex::user()   GraphAycVerted* New graph node
    //  m_breakGraph
    //    GraphEdge::user()     OrigEdgeList*   Old graph edges
    //    GraphVertex::user     bool            Detection of loops in simplifyDupIterate
    // MEMBERS
    V3_ਗ੍ਰਾਫ* ਅਚਲ m_origGraphp;  // Original graph
    V3_ਗ੍ਰਾਫ m_break_ਗ੍ਰਾਫ;  // Graph with only breakable edges represented
    ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼::List m_work;  // List of vertices with optimization work left
    std::ਸਦਿਸ਼<Orig_ਕਿਨਾਰਾ_List*> m_orig_ਕਿਨਾਰਾ_Delp;  // List of deletions to do when done
    ਅਚਲ V3_ਕਿਨਾਰਾ_Func_P
        m_orig_ਕਿਨਾਰਾ_Funcp;  // Function that says we follow this edge (in original graph)
    uint32_t m_ਰੱਖੋ_ਕਦਮ = 0;  // Number that user() must be equal to to indicate processing

    // METHODS
    ਖਾਲੀ build_ਗ੍ਰਾਫ(V3_ਗ੍ਰਾਫ* origGraphp);
    ਖਾਲੀ build_ਗ੍ਰਾਫ_Iterate(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* overtexp, ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* avertexp);
    ਖਾਲੀ simplify(ਤਾਰਕਿਕ allowCut);
    ਖਾਲੀ simplifyNone(ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* avertexp);
    ਖਾਲੀ simplifyOne(ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* avertexp);
    ਖਾਲੀ simplify_ਬਾਹਰਲਾ(ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* avertexp);
    ਖਾਲੀ simplifyDup(ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* avertexp);
    ਖਾਲੀ cutBasic(ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* avertexp);
    ਖਾਲੀ cutBackward(ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* avertexp);
    ਖਾਲੀ deleteMarked();
    ਖਾਲੀ ਰੱਖੋ_2();
    ਖਾਲੀ ਰੱਖੋ_Try_ਕਿਨਾਰਾ(V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* edgep);
    ਤਾਰਕਿਕ ਰੱਖੋ_Iterate(ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* vertexp, uint32_t currentRank);

    ਤਾਰਕਿਕ orig_Follow_ਕਿਨਾਰਾ(V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* edgep) {
        ਵਾਪਸ (edgep->ਭਾਰ_2() && (m_orig_ਕਿਨਾਰਾ_Funcp)(edgep));
    }
    ਖਾਲੀ ਕਿਨਾਰਾ_From_ਕਿਨਾਰਾ(V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* oldedgep, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ) {
        // Make new breakGraph edge, with old edge as a template
        ਗ੍ਰਾਫ_Acyc_ਕਿਨਾਰਾ* ਅਚਲ newEdgep = ਨਵਾਂ ਗ੍ਰਾਫ_Acyc_ਕਿਨਾਰਾ{&m_break_ਗ੍ਰਾਫ, fromp, ਸਿਖਰ,
                                                          oldedgep->ਭਾਰ_2(), oldedgep->cutable()};
        newEdgep->userp(oldedgep->userp());  // Keep pointer to OrigEdgeList
    }
    ਖਾਲੀ ਜੋੜੋ_Orig_Edgep(V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਮੰਜ਼ਿਲ_Edgep, V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਜੋੜੋ_Edgep) {
        // Add addEdge (or it's list) to list of edges that break edge represents
        // Note addEdge may already have a bunch of similar linked edge representations.  Yuk.
        UASSERT(ਜੋੜੋ_Edgep, "Adding nullptr");
        ਜੇ (!ਮੰਜ਼ਿਲ_Edgep->userp()) {
            Orig_ਕਿਨਾਰਾ_List* ਅਚਲ oep = ਨਵਾਂ Orig_ਕਿਨਾਰਾ_List;
            m_orig_ਕਿਨਾਰਾ_Delp.ਧੱਕੋ_ਪਿੱਠ(oep);
            ਮੰਜ਼ਿਲ_Edgep->userp(oep);
        }
        Orig_ਕਿਨਾਰਾ_List* ਅਚਲ oEListp = static_cast<Orig_ਕਿਨਾਰਾ_List*>(ਮੰਜ਼ਿਲ_Edgep->userp());
        ਜੇ (Orig_ਕਿਨਾਰਾ_List* ਅਚਲ ਜੋੜੋ_Listp = static_cast<Orig_ਕਿਨਾਰਾ_List*>(ਜੋੜੋ_Edgep->userp())) {
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : *ਜੋੜੋ_Listp) oEListp->ਧੱਕੋ_ਪਿੱਠ(itr);
            ਜੋੜੋ_Listp->clear();  // Done with it
        } ਨਹੀਂਤਾਂ {
            oEListp->ਧੱਕੋ_ਪਿੱਠ(ਜੋੜੋ_Edgep);
        }
    }
    ਖਾਲੀ cut_Orig_ਕਿਨਾਰਾ(V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* breakEdgep, ਅਚਲ ਅੱਖਰ* why) {
        // From the break edge, cut edges in original graph it represents
        UINFO(8, why << " CUT " << breakEdgep->fromp());
        breakEdgep->cut();
        ਅਚਲ Orig_ਕਿਨਾਰਾ_List* ਅਚਲ oEListp = static_cast<Orig_ਕਿਨਾਰਾ_List*>(breakEdgep->userp());
        ਜੇ (!oEListp) {
            v3fatalSrc("No original edge associated with cutting edge " << breakEdgep);
        }
        // The breakGraph edge may represent multiple real edges; cut them all
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& origEdgep : *oEListp) {
            origEdgep->cut();
            UINFO(8, "  " << why << "   " << origEdgep->fromp() << " ->" << origEdgep->ਸਿਖਰ());
        }
    }
    // Work Queue
    ਖਾਲੀ work_ਧੱਕੋ(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp) {
        ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* ਅਚਲ avertexp = static_cast<ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼*>(vertexp);
        // Add vertex to list of nodes needing further optimization trials
        ਜੇ (!avertexp->m_onWorkList) {
            avertexp->m_onWorkList = ਸੱਚ;
            m_work.link_ਪਿੱਠ(avertexp);
        }
    }
    ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* workBeginp() { ਵਾਪਸ m_work.frontp(); }
    ਖਾਲੀ work_ਕੱਢੋ() {
        ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* ਅਚਲ avertexp = workBeginp();
        avertexp->m_onWorkList = ਝੂਠ;
        m_work.unlink(avertexp);
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਗ੍ਰਾਫ_Acyc(V3_ਗ੍ਰਾਫ* origGraphp, V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp)
        : m_origGraphp{origGraphp}
        , m_orig_ਕਿਨਾਰਾ_Funcp{ਕਿਨਾਰਾ_Funcp} {}
    ~ਗ੍ਰਾਫ_Acyc() {
        ਲਈ (Orig_ਕਿਨਾਰਾ_List* ip : m_orig_ਕਿਨਾਰਾ_Delp) ਮਿਟਾਓ ip;
        m_orig_ਕਿਨਾਰਾ_Delp.clear();
    }
    ਖਾਲੀ ਮੁੱਖ();
};

//--------------------------------------------------------------------

ਖਾਲੀ ਗ੍ਰਾਫ_Acyc::build_ਗ੍ਰਾਫ(V3_ਗ੍ਰਾਫ* origGraphp) {
    // Presumes the graph has been strongly ordered,
    // and thus there's a unique color if there are loops in this subgraph.

    // For each old node, make a new graph node for optimization
    origGraphp->userClearVertices();
    origGraphp->user_Clear_ਕਿਨਾਰੇ();
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& overtex : origGraphp->vertices()) {
        ਜੇ (overtex.color()) {
            ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* ਅਚਲ avertexp = ਨਵਾਂ ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼{&m_break_ਗ੍ਰਾਫ, &overtex};
            overtex.userp(avertexp);  // Stash so can look up later
        }
    }

    // Build edges between logic vertices
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& overtex : origGraphp->vertices()) {
        ਜੇ (overtex.color()) {
            ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* ਅਚਲ avertexp = static_cast<ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼*>(overtex.userp());
            build_ਗ੍ਰਾਫ_Iterate(&overtex, avertexp);
        }
    }
}

ਖਾਲੀ ਗ੍ਰਾਫ_Acyc::build_ਗ੍ਰਾਫ_Iterate(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* overtexp, ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* avertexp) {
    // Make new edges
    ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : overtexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
        ਜੇ (orig_Follow_ਕਿਨਾਰਾ(&ਕਿਨਾਰਾ)) {  // not cut
            ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਮੰਜ਼ਿਲ_Vertexp = ਕਿਨਾਰਾ.ਸਿਖਰ();
            ਜੇ (ਮੰਜ਼ਿਲ_Vertexp->color()) {
                ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* ਅਚਲ ਮੰਜ਼ਿਲ_AVertexp
                    = static_cast<ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼*>(ਮੰਜ਼ਿਲ_Vertexp->userp());
                // Replicate the old edge into the new graph
                // There may be multiple edges between same pairs of vertices
                V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* breakEdgep = ਨਵਾਂ ਗ੍ਰਾਫ_Acyc_ਕਿਨਾਰਾ{&m_break_ਗ੍ਰਾਫ, avertexp, ਮੰਜ਼ਿਲ_AVertexp,
                                                            ਕਿਨਾਰਾ.ਭਾਰ_2(), ਕਿਨਾਰਾ.cutable()};
                ਜੋੜੋ_Orig_Edgep(breakEdgep, &ਕਿਨਾਰਾ);  // So can find original edge
            }
        }
    }
}

ਖਾਲੀ ਗ੍ਰਾਫ_Acyc::simplify(ਤਾਰਕਿਕ allowCut) {
    // Add all nodes to list of work to do
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : m_break_ਗ੍ਰਾਫ.vertices()) work_ਧੱਕੋ(&ਸ਼ੀਰਸ਼);
    // Optimize till everything finished
    ਜਦੋਂ (ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* vertexp = workBeginp()) {
        work_ਕੱਢੋ();
        simplifyNone(vertexp);
        simplifyOne(vertexp);
        simplify_ਬਾਹਰਲਾ(vertexp);
        simplifyDup(vertexp);
        ਜੇ (allowCut) {
            // The main algorithm works without these, though slower
            // So if changing the main algorithm, comment these out for a test run
            ਜੇ (v3Global.opt.fAcycSimp()) {
                cutBasic(vertexp);
                cutBackward(vertexp);
            }
        }
    }
    deleteMarked();
}

ਖਾਲੀ ਗ੍ਰਾਫ_Acyc::deleteMarked() {
    // Delete nodes marked for removal
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_break_ਗ੍ਰਾਫ.vertices().unlinkable()) {
        ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* ਅਚਲ avertexp = static_cast<ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼*>(vtxp);
        ਜੇ (avertexp->isDelete()) {
            VL_DO_DANGLING(avertexp->unlinkDelete(&m_break_ਗ੍ਰਾਫ), avertexp);
        }
    }
}

ਖਾਲੀ ਗ੍ਰਾਫ_Acyc::simplifyNone(ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* avertexp) {
    // Don't need any vertices with no inputs, There's no way they can have a loop.
    // Likewise, vertices with no outputs
    ਜੇ (avertexp->isDelete()) ਵਾਪਸ;
    ਜੇ (avertexp->ਵਿੱਚ_ਖਾਲੀ_ਥਾਵਾਂ() || avertexp->ਬਾਹਰਲਾ_ਖਾਲੀ_ਥਾਵਾਂ()) {
        UINFO(9, "  SimplifyNoneRemove " << avertexp);
        avertexp->ਸੈੱਟ_Delete();  // Mark so we won't delete it twice
        // Remove edges
        ਜਦੋਂ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep = avertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().frontp()) {
            work_ਧੱਕੋ(edgep->ਸਿਖਰ());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        ਜਦੋਂ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep = avertexp->ਵਿੱਚ_ਕਿਨਾਰੇ().frontp()) {
            work_ਧੱਕੋ(edgep->fromp());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
}

ਖਾਲੀ ਗ੍ਰਾਫ_Acyc::simplifyOne(ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* avertexp) {
    // If a node has one input and one output, we can remove it and change the edges
    ਜੇ (avertexp->isDelete()) ਵਾਪਸ;
    ਜੇ (avertexp->ਵਿੱਚ_Size1() && avertexp->ਬਾਹਰਲਾ_Size1()) {
        V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ ਵਿੱਚ_Edgep = avertexp->ਵਿੱਚ_ਕਿਨਾਰੇ().frontp();
        V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ ਬਾਹਰਲਾ_Edgep = avertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().frontp();
        V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ ਵਿੱਚ_Vertexp = ਵਿੱਚ_Edgep->fromp();
        V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ ਬਾਹਰਲਾ_Vertexp = ਬਾਹਰਲਾ_Edgep->ਸਿਖਰ();
        // The in and out may be the same node; we'll make a loop
        // The in OR out may be THIS node; we can't delete it then.
        ਜੇ (ਵਿੱਚ_Vertexp != avertexp && ਬਾਹਰਲਾ_Vertexp != avertexp) {
            UINFO(9, "  SimplifyOneRemove " << avertexp);
            avertexp->ਸੈੱਟ_Delete();  // Mark so we won't delete it twice
            // Make a new edge connecting the two vertices directly
            // If both are breakable, we pick the one with less weight, else it's arbitrary
            // We can forget about the origEdge list for the "non-selected" set of edges,
            // as we need to break only one set or the other set of edges, not both.
            // (This is why we must give preference to the cutable set.)
            V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ templateEdgep
                = ((ਵਿੱਚ_Edgep->cutable()
                    && (!ਬਾਹਰਲਾ_Edgep->cutable() || ਵਿੱਚ_Edgep->ਭਾਰ_2() < ਬਾਹਰਲਾ_Edgep->ਭਾਰ_2()))
                       ? ਵਿੱਚ_Edgep
                       : ਬਾਹਰਲਾ_Edgep);
            // cppcheck-suppress leakReturnValNotUsed
            ਕਿਨਾਰਾ_From_ਕਿਨਾਰਾ(templateEdgep, ਵਿੱਚ_Vertexp, ਬਾਹਰਲਾ_Vertexp);
            // Remove old edge
            VL_DO_DANGLING(ਵਿੱਚ_Edgep->unlinkDelete(), ਵਿੱਚ_Edgep);
            VL_DO_DANGLING(ਬਾਹਰਲਾ_Edgep->unlinkDelete(), ਬਾਹਰਲਾ_Edgep);
            VL_DANGLING(templateEdgep);
            work_ਧੱਕੋ(ਵਿੱਚ_Vertexp);
            work_ਧੱਕੋ(ਬਾਹਰਲਾ_Vertexp);
        }
    }
}

ਖਾਲੀ ਗ੍ਰਾਫ_Acyc::simplify_ਬਾਹਰਲਾ(ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* avertexp) {
    // If a node has one output that's not cutable, all its inputs can be reassigned
    // to the next node in the list
    ਜੇ (avertexp->isDelete()) ਵਾਪਸ;
    ਜੇ (avertexp->ਬਾਹਰਲਾ_Size1()) {
        V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ ਬਾਹਰਲਾ_Edgep = avertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().frontp();
        ਜੇ (!ਬਾਹਰਲਾ_Edgep->cutable()) {
            V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਬਾਹਰਲਾ_Vertexp = ਬਾਹਰਲਾ_Edgep->ਸਿਖਰ();
            UINFO(9, "  SimplifyOutRemove " << avertexp);
            avertexp->ਸੈੱਟ_Delete();  // Mark so we won't delete it twice
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ ਵਿੱਚ_Edgep : avertexp->ਵਿੱਚ_ਕਿਨਾਰੇ().unlinkable()) {
                V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਵਿੱਚ_Vertexp = ਵਿੱਚ_Edgep->fromp();
                ਜੇ (ਵਿੱਚ_Vertexp == avertexp) {
                    ਜੇ (debug()) v3error("Non-cutable vertex=" << avertexp);  // LCOV_EXCL_LINE
                    v3error("Circular logic when ordering code (non-cutable edge loop)\n"
                            << m_origGraphp->reportLoops(  // calls OrderGraph::loopsVertexCb
                                   &V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followNotCutable, avertexp->origVertexp()));
                    // Things are unlikely to end well at this point,
                    // but we'll try something to get to further errors...
                    ਵਿੱਚ_Edgep->cutable(ਸੱਚ);
                    ਵਾਪਸ;
                }
                // Make a new edge connecting the two vertices directly
                // cppcheck-suppress leakReturnValNotUsed
                ਕਿਨਾਰਾ_From_ਕਿਨਾਰਾ(ਵਿੱਚ_Edgep, ਵਿੱਚ_Vertexp, ਬਾਹਰਲਾ_Vertexp);
                // Remove old edge
                VL_DO_DANGLING(ਵਿੱਚ_Edgep->unlinkDelete(), ਵਿੱਚ_Edgep);
                work_ਧੱਕੋ(ਵਿੱਚ_Vertexp);
            }
            VL_DO_DANGLING(ਬਾਹਰਲਾ_Edgep->unlinkDelete(), ਬਾਹਰਲਾ_Edgep);
            work_ਧੱਕੋ(ਬਾਹਰਲਾ_Vertexp);
        }
    }
}

ਖਾਲੀ ਗ੍ਰਾਫ_Acyc::simplifyDup(ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* avertexp) {
    // Remove redundant edges
    ਜੇ (avertexp->isDelete()) ਵਾਪਸ;
    // Clear marks
    ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : avertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) ਕਿਨਾਰਾ.ਸਿਖਰ()->userp(ਖਾਲੀ_ਸੂਚਕ);
    // Mark edges and detect duplications
    ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep : avertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().unlinkable()) {
        V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਬਾਹਰਲਾ_Vertexp = edgep->ਸਿਖਰ();
        V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* prevEdgep = static_cast<V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ*>(ਬਾਹਰਲਾ_Vertexp->userp());
        ਜੇ (prevEdgep) {
            ਜੇ (!prevEdgep->cutable()) {
                // !cutable duplicates prev !cutable: we can ignore it, redundant
                //  cutable duplicates prev !cutable: know it's not a relevant loop, ignore it
                UINFO(8, "    DelDupEdge " << avertexp << " -> " << edgep->ਸਿਖਰ());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            } ਨਹੀਂਤਾਂ ਜੇ (!edgep->cutable()) {
                // !cutable duplicates prev  cutable: delete the earlier cutable
                UINFO(8, "    DelDupPrev " << avertexp << " -> " << prevEdgep->ਸਿਖਰ());
                VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                ਬਾਹਰਲਾ_Vertexp->userp(edgep);
            } ਨਹੀਂਤਾਂ {
                //  cutable duplicates prev  cutable: combine weights
                UINFO(8, "    DelDupComb " << avertexp << " -> " << edgep->ਸਿਖਰ());
                prevEdgep->ਭਾਰ_2(prevEdgep->ਭਾਰ_2() + edgep->ਭਾਰ_2());
                ਜੋੜੋ_Orig_Edgep(prevEdgep, edgep);
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            work_ਧੱਕੋ(ਬਾਹਰਲਾ_Vertexp);
            work_ਧੱਕੋ(avertexp);
        } ਨਹੀਂਤਾਂ {
            // No previous assignment
            ਬਾਹਰਲਾ_Vertexp->userp(edgep);
        }
    }
}

ਖਾਲੀ ਗ੍ਰਾਫ_Acyc::cutBasic(ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* avertexp) {
    // Detect and cleanup any loops from node to itself
    ਜੇ (avertexp->isDelete()) ਵਾਪਸ;
    ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep : avertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().unlinkable()) {
        ਜੇ (edgep->cutable() && edgep->ਸਿਖਰ() == avertexp) {
            cut_Orig_ਕਿਨਾਰਾ(edgep, "  Cut Basic");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_ਧੱਕੋ(avertexp);
        }
    }
}

ਖਾਲੀ ਗ੍ਰਾਫ_Acyc::cutBackward(ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* avertexp) {
    // If a cutable edge is from A->B, and there's a non-cutable edge B->A, then must cut!
    ਜੇ (avertexp->isDelete()) ਵਾਪਸ;
    // Clear marks
    ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : avertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) ਕਿਨਾਰਾ.ਸਿਖਰ()->user(ਝੂਠ);
    ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : avertexp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
        ਜੇ (!ਕਿਨਾਰਾ.cutable()) ਕਿਨਾਰਾ.fromp()->user(ਸੱਚ);
    }
    // Detect duplications
    ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep : avertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().unlinkable()) {
        ਜੇ (edgep->cutable() && edgep->ਸਿਖਰ()->user()) {
            cut_Orig_ਕਿਨਾਰਾ(edgep, "  Cut A->B->A");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_ਧੱਕੋ(avertexp);
        }
    }
}

ਖਾਲੀ ਗ੍ਰਾਫ_Acyc::ਰੱਖੋ_2() {
    // Input is m_breakGraph with ranks already assigned on non-breakable edges

    // Make a list of all cutable edges in the graph
    ਪੂਰਨ ਅੰਕ_ਕਿਨਾਰੇ = 0;
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : m_break_ਗ੍ਰਾਫ.vertices()) {
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : ਸ਼ੀਰਸ਼.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            ਜੇ (ਕਿਨਾਰਾ.ਭਾਰ_2() && ਕਿਨਾਰਾ.cutable()) ++ਅੰਕ_ਕਿਨਾਰੇ;
        }
    }
    UINFO(4, "    Cutable edges = " << ਅੰਕ_ਕਿਨਾਰੇ);

    std::ਸਦਿਸ਼<V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ*> ਕਿਨਾਰੇ;  // List of all edges to be processed
    // Make the vector properly sized right off the bat -- faster than reallocating
    ਕਿਨਾਰੇ.reserve(ਅੰਕ_ਕਿਨਾਰੇ + 1);
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : m_break_ਗ੍ਰਾਫ.vertices()) {
        ਸ਼ੀਰਸ਼.user(0);  // Clear in prep of next step
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : ਸ਼ੀਰਸ਼.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            ਜੇ (ਕਿਨਾਰਾ.ਭਾਰ_2() && ਕਿਨਾਰਾ.cutable()) ਕਿਨਾਰੇ.ਧੱਕੋ_ਪਿੱਠ(&ਕਿਨਾਰਾ);
        }
    }

    // Sort by weight, then by vertex (so that we completely process one vertex, when possible)
    stable_ਛਾਂਟੋ(ਕਿਨਾਰੇ.begin(), ਕਿਨਾਰੇ.ਸਿਰਾ(), ਗ੍ਰਾਫ_Acyc_ਕਿਨਾਰਾ_Cmp());

    // Process each edge in weighted order
    m_ਰੱਖੋ_ਕਦਮ = 10;
    ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* edgep : ਕਿਨਾਰੇ) ਰੱਖੋ_Try_ਕਿਨਾਰਾ(edgep);
}

ਖਾਲੀ ਗ੍ਰਾਫ_Acyc::ਰੱਖੋ_Try_ਕਿਨਾਰਾ(V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* edgep) {
    // Try to make this edge uncutable
    m_ਰੱਖੋ_ਕਦਮ++;
    UINFO(8, "    PlaceEdge s" << m_ਰੱਖੋ_ਕਦਮ << " w" << edgep->ਭਾਰ_2() << " " << edgep->fromp());
    // Make the edge uncutable so we detect it in placement
    edgep->cutable(ਝੂਠ);
    // Vertex::m_user begin: number indicates this edge was completed
    // Try to assign ranks, presuming this edge is in place
    // If we come across user()==placestep, we've detected a loop and must back out
    ਅਚਲ ਤਾਰਕਿਕ loop
        = ਰੱਖੋ_Iterate(static_cast<ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼*>(edgep->ਸਿਖਰ()), edgep->fromp()->rank() + 1);
    ਜੇ (!loop) {
        // No loop, we can keep it as uncutable
        // Commit the new ranks we calculated
        // Just cleanup the list.  If this is slow, we can add another set of
        // user counters to avoid cleaning up the list.
        ਜਦੋਂ (workBeginp()) work_ਕੱਢੋ();
    } ਨਹੀਂਤਾਂ {
        // Adding this edge would cause a loop, kill it
        edgep->cutable(ਸੱਚ);  // So graph still looks pretty
        cut_Orig_ਕਿਨਾਰਾ(edgep, "  Cut loop");
        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        // Back out the ranks we calculated
        ਜਦੋਂ (ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* vertexp = workBeginp()) {
            work_ਕੱਢੋ();
            vertexp->rank(vertexp->m_storedRank);
        }
    }
}

ਤਾਰਕਿਕ ਗ੍ਰਾਫ_Acyc::ਰੱਖੋ_Iterate(ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼* vertexp, uint32_t currentRank) {
    // Assign rank to each unvisited node
    //   rank() is the "committed rank" of the graph known without loops
    // If larger rank is found, assign it and loop back through
    // If we hit a back node make a list of all loops
    ਜੇ (vertexp->rank() >= currentRank) ਵਾਪਸ ਝੂਠ;  // Already processed it
    ਜੇ (vertexp->user() == m_ਰੱਖੋ_ਕਦਮ) ਵਾਪਸ ਸੱਚ;  // Loop detected
    vertexp->user(m_ਰੱਖੋ_ਕਦਮ);
    // Remember we're changing the rank of this node; might need to back out
    ਜੇ (!vertexp->m_onWorkList) {
        vertexp->m_storedRank = vertexp->rank();
        work_ਧੱਕੋ(vertexp);
    }
    vertexp->rank(currentRank);
    // Follow all edges and increase their ranks
    ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
        ਜੇ (ਕਿਨਾਰਾ.ਭਾਰ_2() && !ਕਿਨਾਰਾ.cutable()) {
            ਜੇ (ਰੱਖੋ_Iterate(static_cast<ਗ੍ਰਾਫ_Acyc_ਸ਼ੀਰਸ਼*>(ਕਿਨਾਰਾ.ਸਿਖਰ()), currentRank + 1)) {
                // We don't need to reset user(); we'll use a different placeStep for the next edge
                ਵਾਪਸ ਸੱਚ;  // Loop detected
            }
        }
    }
    vertexp->user(0);
    ਵਾਪਸ ਝੂਠ;
}

//----- Main algorithm entry point

ਖਾਲੀ ਗ੍ਰਾਫ_Acyc::ਮੁੱਖ() {
    m_break_ਗ੍ਰਾਫ.user_Clear_ਕਿਨਾਰੇ();

    // Color based on possible loops
    m_origGraphp->stronglyConnected(m_orig_ਕਿਨਾਰਾ_Funcp);

    // Make a new graph with vertices that have only a single vertex
    // for each group of old vertices that are interconnected with unbreakable
    // edges (and thus can't represent loops - if we did the unbreakable
    // marking right, anyways)
    build_ਗ੍ਰਾਫ(m_origGraphp);
    ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) m_break_ਗ੍ਰਾਫ.dumpDotFilePrefixed("acyc_pre");

    // Perform simple optimizations before any cuttings
    simplify(ਝੂਠ);
    ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 5) m_break_ਗ੍ਰਾਫ.dumpDotFilePrefixed("acyc_simp");

    UINFO(4, " Cutting trivial loops");
    simplify(ਸੱਚ);
    ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) m_break_ਗ੍ਰਾਫ.dumpDotFilePrefixed("acyc_mid");

    UINFO(4, " Ranking");
    m_break_ਗ੍ਰਾਫ.rank(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followNotCutable);
    ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) m_break_ਗ੍ਰਾਫ.dumpDotFilePrefixed("acyc_rank");

    UINFO(4, " Placement");
    ਰੱਖੋ_2();
    ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) m_break_ਗ੍ਰਾਫ.dumpDotFilePrefixed("acyc_place");

    UINFO(4, " Final Ranking");
    // Only needed to assert there are no loops in completed graph
    m_break_ਗ੍ਰਾਫ.rank(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);
    ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) m_break_ਗ੍ਰਾਫ.dumpDotFilePrefixed("acyc_done");
}

ਖਾਲੀ V3_ਗ੍ਰਾਫ::acyclic(V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp) {
    UINFO(4, "Acyclic");
    ਗ੍ਰਾਫ_Acyc acyc{ਇਹ, ਕਿਨਾਰਾ_Funcp};
    acyc.ਮੁੱਖ();
    UINFO(4, "Acyclic done");
}
