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

#ವ್ಯಾಖ್ಯಾನಿಸು VL_MT_DISABLED_ಸಂಕೇತ_UNIT 1

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <list>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - acyclic
//      Break the minimal number of backward edges to make the graph acyclic

ವರ್ಗ ನಕ್ಷೆ_Acyc_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(ನಕ್ಷೆ_Acyc_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
    // user() is used for various sub-algorithm pieces
    V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ m_origVertexp;  // Pointer to first vertex this represents
ಸಂರಕ್ಷಿತ:
    ಮಿತ್ರ ವರ್ಗ ನಕ್ಷೆ_Acyc;
    V3ListLinks<ನಕ್ಷೆ_Acyc_ಶೃಂಗ> m_links;  // List links to store instances of this class
    uint32_t m_storedRank = 0;  // Rank held until commit to edge placement
    ತಾರ್ಕಿಕ m_onWorkList = ಸುಳ್ಳು;  // True if already on list of work to do
    ತಾರ್ಕಿಕ m_deleted = ಸುಳ್ಳು;  // True if deleted

ಖಾಸಗಿ:
    V3ListLinks<ನಕ್ಷೆ_Acyc_ಶೃಂಗ>& links() { ಹಿಂತಿರುಗಿ m_links; }

ಸಾರ್ವಜನಿಕ:
    // List type to store instances of this class
    ಬಳಸಿ List = V3List<ನಕ್ಷೆ_Acyc_ಶೃಂಗ, &ನಕ್ಷೆ_Acyc_ಶೃಂಗ::links>;

    ನಕ್ಷೆ_Acyc_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, V3_ನಕ್ಷೆ_ಶೃಂಗ* origVertexp)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp}
        , m_origVertexp{origVertexp} {}
    ~ನಕ್ಷೆ_Acyc_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    V3_ನಕ್ಷೆ_ಶೃಂಗ* origVertexp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_origVertexp; }
    ಶೂನ್ಯ ಗಣ_Delete() { m_deleted = ನಿಜ; }
    ತಾರ್ಕಿಕ isDelete() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_deleted; }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string name() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_origVertexp->name(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_origVertexp->dotColor(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_origVertexp->fileline(); }
};

//--------------------------------------------------------------------

ವರ್ಗ ನಕ್ಷೆ_Acyc_ಅಂಚು final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಅಂಚು {
    VL_RTTI_IMPL(ನಕ್ಷೆ_Acyc_ಅಂಚು, V3_ನಕ್ಷೆ_ಅಂಚು)
    // userp() is always used to point to the head original graph edge
ಖಾಸಗಿ:
    ಬಳಸಿ Orig_ಅಂಚು_List = std::list<V3_ನಕ್ಷೆ_ಅಂಚು*>;  // List of orig edges, see also GraphAcyc's decl
    V3_ನಕ್ಷೆ_ಅಂಚು* origEdgep() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ Orig_ಅಂಚು_List* ಸ್ಥಿರಾಂಕ oEListp = static_cast<Orig_ಅಂಚು_List*>(userp());
        UASSERT(oEListp, "No original edge associated with acyc edge " << ಇದು);
        ಹಿಂತಿರುಗಿ (oEListp->front());
    }

ಸಾರ್ವಜನಿಕ:
    ನಕ್ಷೆ_Acyc_ಅಂಚು(V3_ನಕ್ಷೆ* graphp, V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp, V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ, ಪೂರ್ಣಾಂಕ ತೂಕ_2,
                  ತಾರ್ಕಿಕ cutable = ಸುಳ್ಳು)
        : V3_ನಕ್ಷೆ_ಅಂಚು{graphp, fromp, ತುದಿ, ತೂಕ_2, cutable} {}
    ~ನಕ್ಷೆ_Acyc_ಅಂಚು() override = ಪೂರ್ವನಿಯೋಜಿತ;
    // yellow=we might still cut it, else oldEdge: yellowGreen=made uncutable, red=uncutable
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ (cutable() ? "yellow" : origEdgep()->dotColor()); }
};

//--------------------------------------------------------------------

ರಚನೆ ನಕ್ಷೆ_Acyc_ಅಂಚು_Cmp final {
    ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು* lhsp, ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು* rhsp) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (lhsp->ತೂಕ_2() > rhsp->ತೂಕ_2()) ಹಿಂತಿರುಗಿ ನಿಜ;  // LHS goes first
        ಆದರೆ (lhsp->ತೂಕ_2() < rhsp->ತೂಕ_2()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // RHS goes first
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
};

//--------------------------------------------------------------------

// CLASSES
ವರ್ಗ ನಕ್ಷೆ_Acyc final {
    ಬಳಸಿ Orig_ಅಂಚು_List
        = std::list<V3_ನಕ್ಷೆ_ಅಂಚು*>;  // List of orig edges, see also GraphAcycEdge's decl
    // GRAPH USERS
    //  origGraph
    //    GraphVertex::user()   GraphAycVerted* New graph node
    //  m_breakGraph
    //    GraphEdge::user()     OrigEdgeList*   Old graph edges
    //    GraphVertex::user     bool            Detection of loops in simplifyDupIterate
    // MEMBERS
    V3_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ m_origGraphp;  // Original graph
    V3_ನಕ್ಷೆ m_break_ನಕ್ಷೆ;  // Graph with only breakable edges represented
    ನಕ್ಷೆ_Acyc_ಶೃಂಗ::List m_work;  // List of vertices with optimization work left
    std::ಸದಿಶ<Orig_ಅಂಚು_List*> m_orig_ಅಂಚು_Delp;  // List of deletions to do when done
    ಸ್ಥಿರಾಂಕ V3_ಅಂಚು_Func_P
        m_orig_ಅಂಚು_Funcp;  // Function that says we follow this edge (in original graph)
    uint32_t m_ಇರಿಸು_ಹೆಜ್ಜೆ = 0;  // Number that user() must be equal to to indicate processing

    // METHODS
    ಶೂನ್ಯ build_ನಕ್ಷೆ(V3_ನಕ್ಷೆ* origGraphp);
    ಶೂನ್ಯ build_ನಕ್ಷೆ_Iterate(V3_ನಕ್ಷೆ_ಶೃಂಗ* overtexp, ನಕ್ಷೆ_Acyc_ಶೃಂಗ* avertexp);
    ಶೂನ್ಯ simplify(ತಾರ್ಕಿಕ allowCut);
    ಶೂನ್ಯ simplifyNone(ನಕ್ಷೆ_Acyc_ಶೃಂಗ* avertexp);
    ಶೂನ್ಯ simplifyOne(ನಕ್ಷೆ_Acyc_ಶೃಂಗ* avertexp);
    ಶೂನ್ಯ simplify_ಹೊರಹೋಗು(ನಕ್ಷೆ_Acyc_ಶೃಂಗ* avertexp);
    ಶೂನ್ಯ simplifyDup(ನಕ್ಷೆ_Acyc_ಶೃಂಗ* avertexp);
    ಶೂನ್ಯ cutBasic(ನಕ್ಷೆ_Acyc_ಶೃಂಗ* avertexp);
    ಶೂನ್ಯ cutBackward(ನಕ್ಷೆ_Acyc_ಶೃಂಗ* avertexp);
    ಶೂನ್ಯ deleteMarked();
    ಶೂನ್ಯ ಇರಿಸು_2();
    ಶೂನ್ಯ ಇರಿಸು_Try_ಅಂಚು(V3_ನಕ್ಷೆ_ಅಂಚು* edgep);
    ತಾರ್ಕಿಕ ಇರಿಸು_Iterate(ನಕ್ಷೆ_Acyc_ಶೃಂಗ* vertexp, uint32_t currentRank);

    ತಾರ್ಕಿಕ orig_Follow_ಅಂಚು(V3_ನಕ್ಷೆ_ಅಂಚು* edgep) {
        ಹಿಂತಿರುಗಿ (edgep->ತೂಕ_2() && (m_orig_ಅಂಚು_Funcp)(edgep));
    }
    ಶೂನ್ಯ ಅಂಚು_From_ಅಂಚು(V3_ನಕ್ಷೆ_ಅಂಚು* oldedgep, V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp, V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ) {
        // Make new breakGraph edge, with old edge as a template
        ನಕ್ಷೆ_Acyc_ಅಂಚು* ಸ್ಥಿರಾಂಕ newEdgep = ಹೊಸ ನಕ್ಷೆ_Acyc_ಅಂಚು{&m_break_ನಕ್ಷೆ, fromp, ತುದಿ,
                                                          oldedgep->ತೂಕ_2(), oldedgep->cutable()};
        newEdgep->userp(oldedgep->userp());  // Keep pointer to OrigEdgeList
    }
    ಶೂನ್ಯ ಕೂಡಿಸು_Orig_Edgep(V3_ನಕ್ಷೆ_ಅಂಚು* ಗಮ್ಯ_Edgep, V3_ನಕ್ಷೆ_ಅಂಚು* ಕೂಡಿಸು_Edgep) {
        // Add addEdge (or it's list) to list of edges that break edge represents
        // Note addEdge may already have a bunch of similar linked edge representations.  Yuk.
        UASSERT(ಕೂಡಿಸು_Edgep, "Adding nullptr");
        ಆದರೆ (!ಗಮ್ಯ_Edgep->userp()) {
            Orig_ಅಂಚು_List* ಸ್ಥಿರಾಂಕ oep = ಹೊಸ Orig_ಅಂಚು_List;
            m_orig_ಅಂಚು_Delp.ತಳ್ಳು_ಹಿಂಭಾಗ(oep);
            ಗಮ್ಯ_Edgep->userp(oep);
        }
        Orig_ಅಂಚು_List* ಸ್ಥಿರಾಂಕ oEListp = static_cast<Orig_ಅಂಚು_List*>(ಗಮ್ಯ_Edgep->userp());
        ಆದರೆ (Orig_ಅಂಚು_List* ಸ್ಥಿರಾಂಕ ಕೂಡಿಸು_Listp = static_cast<Orig_ಅಂಚು_List*>(ಕೂಡಿಸು_Edgep->userp())) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : *ಕೂಡಿಸು_Listp) oEListp->ತಳ್ಳು_ಹಿಂಭಾಗ(itr);
            ಕೂಡಿಸು_Listp->clear();  // Done with it
        } ಇಲ್ಲದಿದ್ದರೆ {
            oEListp->ತಳ್ಳು_ಹಿಂಭಾಗ(ಕೂಡಿಸು_Edgep);
        }
    }
    ಶೂನ್ಯ cut_Orig_ಅಂಚು(V3_ನಕ್ಷೆ_ಅಂಚು* breakEdgep, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* why) {
        // From the break edge, cut edges in original graph it represents
        UINFO(8, why << " CUT " << breakEdgep->fromp());
        breakEdgep->cut();
        ಸ್ಥಿರಾಂಕ Orig_ಅಂಚು_List* ಸ್ಥಿರಾಂಕ oEListp = static_cast<Orig_ಅಂಚು_List*>(breakEdgep->userp());
        ಆದರೆ (!oEListp) {
            v3fatalSrc("No original edge associated with cutting edge " << breakEdgep);
        }
        // The breakGraph edge may represent multiple real edges; cut them all
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& origEdgep : *oEListp) {
            origEdgep->cut();
            UINFO(8, "  " << why << "   " << origEdgep->fromp() << " ->" << origEdgep->ತುದಿ());
        }
    }
    // Work Queue
    ಶೂನ್ಯ work_ತಳ್ಳು(V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp) {
        ನಕ್ಷೆ_Acyc_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ avertexp = static_cast<ನಕ್ಷೆ_Acyc_ಶೃಂಗ*>(vertexp);
        // Add vertex to list of nodes needing further optimization trials
        ಆದರೆ (!avertexp->m_onWorkList) {
            avertexp->m_onWorkList = ನಿಜ;
            m_work.link_ಹಿಂಭಾಗ(avertexp);
        }
    }
    ನಕ್ಷೆ_Acyc_ಶೃಂಗ* workBeginp() { ಹಿಂತಿರುಗಿ m_work.frontp(); }
    ಶೂನ್ಯ work_ತೆಗೆ() {
        ನಕ್ಷೆ_Acyc_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ avertexp = workBeginp();
        avertexp->m_onWorkList = ಸುಳ್ಳು;
        m_work.unlink(avertexp);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ನಕ್ಷೆ_Acyc(V3_ನಕ್ಷೆ* origGraphp, V3_ಅಂಚು_Func_P ಅಂಚು_Funcp)
        : m_origGraphp{origGraphp}
        , m_orig_ಅಂಚು_Funcp{ಅಂಚು_Funcp} {}
    ~ನಕ್ಷೆ_Acyc() {
        ಗಾಗಿ (Orig_ಅಂಚು_List* ip : m_orig_ಅಂಚು_Delp) ಅಳಿಸು ip;
        m_orig_ಅಂಚು_Delp.clear();
    }
    ಶೂನ್ಯ ಮುಖ್ಯ();
};

//--------------------------------------------------------------------

ಶೂನ್ಯ ನಕ್ಷೆ_Acyc::build_ನಕ್ಷೆ(V3_ನಕ್ಷೆ* origGraphp) {
    // Presumes the graph has been strongly ordered,
    // and thus there's a unique color if there are loops in this subgraph.

    // For each old node, make a new graph node for optimization
    origGraphp->userClearVertices();
    origGraphp->user_Clear_ಅಂಚುಗಳು();
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& overtex : origGraphp->vertices()) {
        ಆದರೆ (overtex.color()) {
            ನಕ್ಷೆ_Acyc_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ avertexp = ಹೊಸ ನಕ್ಷೆ_Acyc_ಶೃಂಗ{&m_break_ನಕ್ಷೆ, &overtex};
            overtex.userp(avertexp);  // Stash so can look up later
        }
    }

    // Build edges between logic vertices
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& overtex : origGraphp->vertices()) {
        ಆದರೆ (overtex.color()) {
            ನಕ್ಷೆ_Acyc_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ avertexp = static_cast<ನಕ್ಷೆ_Acyc_ಶೃಂಗ*>(overtex.userp());
            build_ನಕ್ಷೆ_Iterate(&overtex, avertexp);
        }
    }
}

ಶೂನ್ಯ ನಕ್ಷೆ_Acyc::build_ನಕ್ಷೆ_Iterate(V3_ನಕ್ಷೆ_ಶೃಂಗ* overtexp, ನಕ್ಷೆ_Acyc_ಶೃಂಗ* avertexp) {
    // Make new edges
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : overtexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
        ಆದರೆ (orig_Follow_ಅಂಚು(&ಅಂಚು)) {  // not cut
            ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* ಗಮ್ಯ_Vertexp = ಅಂಚು.ತುದಿ();
            ಆದರೆ (ಗಮ್ಯ_Vertexp->color()) {
                ನಕ್ಷೆ_Acyc_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ಗಮ್ಯ_AVertexp
                    = static_cast<ನಕ್ಷೆ_Acyc_ಶೃಂಗ*>(ಗಮ್ಯ_Vertexp->userp());
                // Replicate the old edge into the new graph
                // There may be multiple edges between same pairs of vertices
                V3_ನಕ್ಷೆ_ಅಂಚು* breakEdgep = ಹೊಸ ನಕ್ಷೆ_Acyc_ಅಂಚು{&m_break_ನಕ್ಷೆ, avertexp, ಗಮ್ಯ_AVertexp,
                                                            ಅಂಚು.ತೂಕ_2(), ಅಂಚು.cutable()};
                ಕೂಡಿಸು_Orig_Edgep(breakEdgep, &ಅಂಚು);  // So can find original edge
            }
        }
    }
}

ಶೂನ್ಯ ನಕ್ಷೆ_Acyc::simplify(ತಾರ್ಕಿಕ allowCut) {
    // Add all nodes to list of work to do
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : m_break_ನಕ್ಷೆ.vertices()) work_ತಳ್ಳು(&ಶೃಂಗ);
    // Optimize till everything finished
    ಆಗ (ನಕ್ಷೆ_Acyc_ಶೃಂಗ* vertexp = workBeginp()) {
        work_ತೆಗೆ();
        simplifyNone(vertexp);
        simplifyOne(vertexp);
        simplify_ಹೊರಹೋಗು(vertexp);
        simplifyDup(vertexp);
        ಆದರೆ (allowCut) {
            // The main algorithm works without these, though slower
            // So if changing the main algorithm, comment these out for a test run
            ಆದರೆ (v3Global.opt.fAcycSimp()) {
                cutBasic(vertexp);
                cutBackward(vertexp);
            }
        }
    }
    deleteMarked();
}

ಶೂನ್ಯ ನಕ್ಷೆ_Acyc::deleteMarked() {
    // Delete nodes marked for removal
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_break_ನಕ್ಷೆ.vertices().unlinkable()) {
        ನಕ್ಷೆ_Acyc_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ avertexp = static_cast<ನಕ್ಷೆ_Acyc_ಶೃಂಗ*>(vtxp);
        ಆದರೆ (avertexp->isDelete()) {
            VL_DO_DANGLING(avertexp->unlinkDelete(&m_break_ನಕ್ಷೆ), avertexp);
        }
    }
}

ಶೂನ್ಯ ನಕ್ಷೆ_Acyc::simplifyNone(ನಕ್ಷೆ_Acyc_ಶೃಂಗ* avertexp) {
    // Don't need any vertices with no inputs, There's no way they can have a loop.
    // Likewise, vertices with no outputs
    ಆದರೆ (avertexp->isDelete()) ಹಿಂತಿರುಗಿ;
    ಆದರೆ (avertexp->ಒಳ_ಖಾಲಿಗಳು() || avertexp->ಹೊರಹೋಗು_ಖಾಲಿಗಳು()) {
        UINFO(9, "  SimplifyNoneRemove " << avertexp);
        avertexp->ಗಣ_Delete();  // Mark so we won't delete it twice
        // Remove edges
        ಆಗ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep = avertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().frontp()) {
            work_ತಳ್ಳು(edgep->ತುದಿ());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        ಆಗ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep = avertexp->ಒಳ_ಅಂಚುಗಳು().frontp()) {
            work_ತಳ್ಳು(edgep->fromp());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
}

ಶೂನ್ಯ ನಕ್ಷೆ_Acyc::simplifyOne(ನಕ್ಷೆ_Acyc_ಶೃಂಗ* avertexp) {
    // If a node has one input and one output, we can remove it and change the edges
    ಆದರೆ (avertexp->isDelete()) ಹಿಂತಿರುಗಿ;
    ಆದರೆ (avertexp->ಒಳ_Size1() && avertexp->ಹೊರಹೋಗು_Size1()) {
        V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ ಒಳ_Edgep = avertexp->ಒಳ_ಅಂಚುಗಳು().frontp();
        V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ ಹೊರಹೋಗು_Edgep = avertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().frontp();
        V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ಒಳ_Vertexp = ಒಳ_Edgep->fromp();
        V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ಹೊರಹೋಗು_Vertexp = ಹೊರಹೋಗು_Edgep->ತುದಿ();
        // The in and out may be the same node; we'll make a loop
        // The in OR out may be THIS node; we can't delete it then.
        ಆದರೆ (ಒಳ_Vertexp != avertexp && ಹೊರಹೋಗು_Vertexp != avertexp) {
            UINFO(9, "  SimplifyOneRemove " << avertexp);
            avertexp->ಗಣ_Delete();  // Mark so we won't delete it twice
            // Make a new edge connecting the two vertices directly
            // If both are breakable, we pick the one with less weight, else it's arbitrary
            // We can forget about the origEdge list for the "non-selected" set of edges,
            // as we need to break only one set or the other set of edges, not both.
            // (This is why we must give preference to the cutable set.)
            V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ templateEdgep
                = ((ಒಳ_Edgep->cutable()
                    && (!ಹೊರಹೋಗು_Edgep->cutable() || ಒಳ_Edgep->ತೂಕ_2() < ಹೊರಹೋಗು_Edgep->ತೂಕ_2()))
                       ? ಒಳ_Edgep
                       : ಹೊರಹೋಗು_Edgep);
            // cppcheck-suppress leakReturnValNotUsed
            ಅಂಚು_From_ಅಂಚು(templateEdgep, ಒಳ_Vertexp, ಹೊರಹೋಗು_Vertexp);
            // Remove old edge
            VL_DO_DANGLING(ಒಳ_Edgep->unlinkDelete(), ಒಳ_Edgep);
            VL_DO_DANGLING(ಹೊರಹೋಗು_Edgep->unlinkDelete(), ಹೊರಹೋಗು_Edgep);
            VL_DANGLING(templateEdgep);
            work_ತಳ್ಳು(ಒಳ_Vertexp);
            work_ತಳ್ಳು(ಹೊರಹೋಗು_Vertexp);
        }
    }
}

ಶೂನ್ಯ ನಕ್ಷೆ_Acyc::simplify_ಹೊರಹೋಗು(ನಕ್ಷೆ_Acyc_ಶೃಂಗ* avertexp) {
    // If a node has one output that's not cutable, all its inputs can be reassigned
    // to the next node in the list
    ಆದರೆ (avertexp->isDelete()) ಹಿಂತಿರುಗಿ;
    ಆದರೆ (avertexp->ಹೊರಹೋಗು_Size1()) {
        V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ ಹೊರಹೋಗು_Edgep = avertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().frontp();
        ಆದರೆ (!ಹೊರಹೋಗು_Edgep->cutable()) {
            V3_ನಕ್ಷೆ_ಶೃಂಗ* ಹೊರಹೋಗು_Vertexp = ಹೊರಹೋಗು_Edgep->ತುದಿ();
            UINFO(9, "  SimplifyOutRemove " << avertexp);
            avertexp->ಗಣ_Delete();  // Mark so we won't delete it twice
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ ಒಳ_Edgep : avertexp->ಒಳ_ಅಂಚುಗಳು().unlinkable()) {
                V3_ನಕ್ಷೆ_ಶೃಂಗ* ಒಳ_Vertexp = ಒಳ_Edgep->fromp();
                ಆದರೆ (ಒಳ_Vertexp == avertexp) {
                    ಆದರೆ (debug()) v3error("Non-cutable vertex=" << avertexp);  // LCOV_EXCL_LINE
                    v3error("Circular logic when ordering code (non-cutable edge loop)\n"
                            << m_origGraphp->reportLoops(  // calls OrderGraph::loopsVertexCb
                                   &V3_ನಕ್ಷೆ_ಅಂಚು::followNotCutable, avertexp->origVertexp()));
                    // Things are unlikely to end well at this point,
                    // but we'll try something to get to further errors...
                    ಒಳ_Edgep->cutable(ನಿಜ);
                    ಹಿಂತಿರುಗಿ;
                }
                // Make a new edge connecting the two vertices directly
                // cppcheck-suppress leakReturnValNotUsed
                ಅಂಚು_From_ಅಂಚು(ಒಳ_Edgep, ಒಳ_Vertexp, ಹೊರಹೋಗು_Vertexp);
                // Remove old edge
                VL_DO_DANGLING(ಒಳ_Edgep->unlinkDelete(), ಒಳ_Edgep);
                work_ತಳ್ಳು(ಒಳ_Vertexp);
            }
            VL_DO_DANGLING(ಹೊರಹೋಗು_Edgep->unlinkDelete(), ಹೊರಹೋಗು_Edgep);
            work_ತಳ್ಳು(ಹೊರಹೋಗು_Vertexp);
        }
    }
}

ಶೂನ್ಯ ನಕ್ಷೆ_Acyc::simplifyDup(ನಕ್ಷೆ_Acyc_ಶೃಂಗ* avertexp) {
    // Remove redundant edges
    ಆದರೆ (avertexp->isDelete()) ಹಿಂತಿರುಗಿ;
    // Clear marks
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : avertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) ಅಂಚು.ತುದಿ()->userp(ಶೂನ್ಯಸೂಚಕ);
    // Mark edges and detect duplications
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep : avertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().unlinkable()) {
        V3_ನಕ್ಷೆ_ಶೃಂಗ* ಹೊರಹೋಗು_Vertexp = edgep->ತುದಿ();
        V3_ನಕ್ಷೆ_ಅಂಚು* prevEdgep = static_cast<V3_ನಕ್ಷೆ_ಅಂಚು*>(ಹೊರಹೋಗು_Vertexp->userp());
        ಆದರೆ (prevEdgep) {
            ಆದರೆ (!prevEdgep->cutable()) {
                // !cutable duplicates prev !cutable: we can ignore it, redundant
                //  cutable duplicates prev !cutable: know it's not a relevant loop, ignore it
                UINFO(8, "    DelDupEdge " << avertexp << " -> " << edgep->ತುದಿ());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!edgep->cutable()) {
                // !cutable duplicates prev  cutable: delete the earlier cutable
                UINFO(8, "    DelDupPrev " << avertexp << " -> " << prevEdgep->ತುದಿ());
                VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                ಹೊರಹೋಗು_Vertexp->userp(edgep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                //  cutable duplicates prev  cutable: combine weights
                UINFO(8, "    DelDupComb " << avertexp << " -> " << edgep->ತುದಿ());
                prevEdgep->ತೂಕ_2(prevEdgep->ತೂಕ_2() + edgep->ತೂಕ_2());
                ಕೂಡಿಸು_Orig_Edgep(prevEdgep, edgep);
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            work_ತಳ್ಳು(ಹೊರಹೋಗು_Vertexp);
            work_ತಳ್ಳು(avertexp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // No previous assignment
            ಹೊರಹೋಗು_Vertexp->userp(edgep);
        }
    }
}

ಶೂನ್ಯ ನಕ್ಷೆ_Acyc::cutBasic(ನಕ್ಷೆ_Acyc_ಶೃಂಗ* avertexp) {
    // Detect and cleanup any loops from node to itself
    ಆದರೆ (avertexp->isDelete()) ಹಿಂತಿರುಗಿ;
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep : avertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().unlinkable()) {
        ಆದರೆ (edgep->cutable() && edgep->ತುದಿ() == avertexp) {
            cut_Orig_ಅಂಚು(edgep, "  Cut Basic");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_ತಳ್ಳು(avertexp);
        }
    }
}

ಶೂನ್ಯ ನಕ್ಷೆ_Acyc::cutBackward(ನಕ್ಷೆ_Acyc_ಶೃಂಗ* avertexp) {
    // If a cutable edge is from A->B, and there's a non-cutable edge B->A, then must cut!
    ಆದರೆ (avertexp->isDelete()) ಹಿಂತಿರುಗಿ;
    // Clear marks
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : avertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) ಅಂಚು.ತುದಿ()->user(ಸುಳ್ಳು);
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : avertexp->ಒಳ_ಅಂಚುಗಳು()) {
        ಆದರೆ (!ಅಂಚು.cutable()) ಅಂಚು.fromp()->user(ನಿಜ);
    }
    // Detect duplications
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep : avertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().unlinkable()) {
        ಆದರೆ (edgep->cutable() && edgep->ತುದಿ()->user()) {
            cut_Orig_ಅಂಚು(edgep, "  Cut A->B->A");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_ತಳ್ಳು(avertexp);
        }
    }
}

ಶೂನ್ಯ ನಕ್ಷೆ_Acyc::ಇರಿಸು_2() {
    // Input is m_breakGraph with ranks already assigned on non-breakable edges

    // Make a list of all cutable edges in the graph
    ಪೂರ್ಣಾಂಕ ಅಂಕೆ_ಅಂಚುಗಳು = 0;
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : m_break_ನಕ್ಷೆ.vertices()) {
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : ಶೃಂಗ.ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            ಆದರೆ (ಅಂಚು.ತೂಕ_2() && ಅಂಚು.cutable()) ++ಅಂಕೆ_ಅಂಚುಗಳು;
        }
    }
    UINFO(4, "    Cutable edges = " << ಅಂಕೆ_ಅಂಚುಗಳು);

    std::ಸದಿಶ<V3_ನಕ್ಷೆ_ಅಂಚು*> ಅಂಚುಗಳು;  // List of all edges to be processed
    // Make the vector properly sized right off the bat -- faster than reallocating
    ಅಂಚುಗಳು.reserve(ಅಂಕೆ_ಅಂಚುಗಳು + 1);
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : m_break_ನಕ್ಷೆ.vertices()) {
        ಶೃಂಗ.user(0);  // Clear in prep of next step
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : ಶೃಂಗ.ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            ಆದರೆ (ಅಂಚು.ತೂಕ_2() && ಅಂಚು.cutable()) ಅಂಚುಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(&ಅಂಚು);
        }
    }

    // Sort by weight, then by vertex (so that we completely process one vertex, when possible)
    stable_ವಿಂಗಡಿಸು(ಅಂಚುಗಳು.begin(), ಅಂಚುಗಳು.ಕೊನೆ(), ನಕ್ಷೆ_Acyc_ಅಂಚು_Cmp());

    // Process each edge in weighted order
    m_ಇರಿಸು_ಹೆಜ್ಜೆ = 10;
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು* edgep : ಅಂಚುಗಳು) ಇರಿಸು_Try_ಅಂಚು(edgep);
}

ಶೂನ್ಯ ನಕ್ಷೆ_Acyc::ಇರಿಸು_Try_ಅಂಚು(V3_ನಕ್ಷೆ_ಅಂಚು* edgep) {
    // Try to make this edge uncutable
    m_ಇರಿಸು_ಹೆಜ್ಜೆ++;
    UINFO(8, "    PlaceEdge s" << m_ಇರಿಸು_ಹೆಜ್ಜೆ << " w" << edgep->ತೂಕ_2() << " " << edgep->fromp());
    // Make the edge uncutable so we detect it in placement
    edgep->cutable(ಸುಳ್ಳು);
    // Vertex::m_user begin: number indicates this edge was completed
    // Try to assign ranks, presuming this edge is in place
    // If we come across user()==placestep, we've detected a loop and must back out
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ loop
        = ಇರಿಸು_Iterate(static_cast<ನಕ್ಷೆ_Acyc_ಶೃಂಗ*>(edgep->ತುದಿ()), edgep->fromp()->rank() + 1);
    ಆದರೆ (!loop) {
        // No loop, we can keep it as uncutable
        // Commit the new ranks we calculated
        // Just cleanup the list.  If this is slow, we can add another set of
        // user counters to avoid cleaning up the list.
        ಆಗ (workBeginp()) work_ತೆಗೆ();
    } ಇಲ್ಲದಿದ್ದರೆ {
        // Adding this edge would cause a loop, kill it
        edgep->cutable(ನಿಜ);  // So graph still looks pretty
        cut_Orig_ಅಂಚು(edgep, "  Cut loop");
        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        // Back out the ranks we calculated
        ಆಗ (ನಕ್ಷೆ_Acyc_ಶೃಂಗ* vertexp = workBeginp()) {
            work_ತೆಗೆ();
            vertexp->rank(vertexp->m_storedRank);
        }
    }
}

ತಾರ್ಕಿಕ ನಕ್ಷೆ_Acyc::ಇರಿಸು_Iterate(ನಕ್ಷೆ_Acyc_ಶೃಂಗ* vertexp, uint32_t currentRank) {
    // Assign rank to each unvisited node
    //   rank() is the "committed rank" of the graph known without loops
    // If larger rank is found, assign it and loop back through
    // If we hit a back node make a list of all loops
    ಆದರೆ (vertexp->rank() >= currentRank) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Already processed it
    ಆದರೆ (vertexp->user() == m_ಇರಿಸು_ಹೆಜ್ಜೆ) ಹಿಂತಿರುಗಿ ನಿಜ;  // Loop detected
    vertexp->user(m_ಇರಿಸು_ಹೆಜ್ಜೆ);
    // Remember we're changing the rank of this node; might need to back out
    ಆದರೆ (!vertexp->m_onWorkList) {
        vertexp->m_storedRank = vertexp->rank();
        work_ತಳ್ಳು(vertexp);
    }
    vertexp->rank(currentRank);
    // Follow all edges and increase their ranks
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
        ಆದರೆ (ಅಂಚು.ತೂಕ_2() && !ಅಂಚು.cutable()) {
            ಆದರೆ (ಇರಿಸು_Iterate(static_cast<ನಕ್ಷೆ_Acyc_ಶೃಂಗ*>(ಅಂಚು.ತುದಿ()), currentRank + 1)) {
                // We don't need to reset user(); we'll use a different placeStep for the next edge
                ಹಿಂತಿರುಗಿ ನಿಜ;  // Loop detected
            }
        }
    }
    vertexp->user(0);
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
}

//----- Main algorithm entry point

ಶೂನ್ಯ ನಕ್ಷೆ_Acyc::ಮುಖ್ಯ() {
    m_break_ನಕ್ಷೆ.user_Clear_ಅಂಚುಗಳು();

    // Color based on possible loops
    m_origGraphp->stronglyConnected(m_orig_ಅಂಚು_Funcp);

    // Make a new graph with vertices that have only a single vertex
    // for each group of old vertices that are interconnected with unbreakable
    // edges (and thus can't represent loops - if we did the unbreakable
    // marking right, anyways)
    build_ನಕ್ಷೆ(m_origGraphp);
    ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) m_break_ನಕ್ಷೆ.dumpDotFilePrefixed("acyc_pre");

    // Perform simple optimizations before any cuttings
    simplify(ಸುಳ್ಳು);
    ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 5) m_break_ನಕ್ಷೆ.dumpDotFilePrefixed("acyc_simp");

    UINFO(4, " Cutting trivial loops");
    simplify(ನಿಜ);
    ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) m_break_ನಕ್ಷೆ.dumpDotFilePrefixed("acyc_mid");

    UINFO(4, " Ranking");
    m_break_ನಕ್ಷೆ.rank(&V3_ನಕ್ಷೆ_ಅಂಚು::followNotCutable);
    ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) m_break_ನಕ್ಷೆ.dumpDotFilePrefixed("acyc_rank");

    UINFO(4, " Placement");
    ಇರಿಸು_2();
    ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) m_break_ನಕ್ಷೆ.dumpDotFilePrefixed("acyc_place");

    UINFO(4, " Final Ranking");
    // Only needed to assert there are no loops in completed graph
    m_break_ನಕ್ಷೆ.rank(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);
    ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) m_break_ನಕ್ಷೆ.dumpDotFilePrefixed("acyc_done");
}

ಶೂನ್ಯ V3_ನಕ್ಷೆ::acyclic(V3_ಅಂಚು_Func_P ಅಂಚು_Funcp) {
    UINFO(4, "Acyclic");
    ನಕ್ಷೆ_Acyc acyc{ಇದು, ಅಂಚು_Funcp};
    acyc.ಮುಖ್ಯ();
    UINFO(4, "Acyclic done");
}
