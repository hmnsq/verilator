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

#تعریف_کړه VL_MT_DISABLED_کوډ_UNIT 1

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Global.h"
#ضمن "V3Graph.h"

#ضمن <algorithm>
#ضمن <list>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - acyclic
//      Break the minimal number of backward edges to make the graph acyclic

صنف ګراف_Acyc_راس final : عام V3_ګراف_راس {
    VL_RTTI_IMPL(ګراف_Acyc_راس, V3_ګراف_راس)
    // user() is used for various sub-algorithm pieces
    V3_ګراف_راس* ثابت m_origVertexp;  // Pointer to first vertex this represents
محمي:
    ملګری صنف ګراف_Acyc;
    V3ListLinks<ګراف_Acyc_راس> m_links;  // List links to store instances of this class
    uint32_t m_storedRank = 0;  // Rank held until commit to edge placement
    منطقي m_onWorkList = خطأ;  // True if already on list of work to do
    منطقي m_deleted = خطأ;  // True if deleted

خاص:
    V3ListLinks<ګراف_Acyc_راس>& links() { أرجع m_links; }

عام:
    // List type to store instances of this class
    کارول List = V3List<ګراف_Acyc_راس, &ګراف_Acyc_راس::links>;

    ګراف_Acyc_راس(V3_ګراف* graphp, V3_ګراف_راس* origVertexp)
        : V3_ګراف_راس{graphp}
        , m_origVertexp{origVertexp} {}
    ~ګراف_Acyc_راس() override = افتراضي;
    V3_ګراف_راس* origVertexp() ثابت { أرجع m_origVertexp; }
    فراغ سېټ_Delete() { m_deleted = ریښتیا; }
    منطقي isDelete() ثابت { أرجع m_deleted; }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string name() ثابت override { أرجع m_origVertexp->name(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string dotColor() ثابت override { أرجع m_origVertexp->dotColor(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    File_کرښه* fileline() ثابت override { أرجع m_origVertexp->fileline(); }
};

//--------------------------------------------------------------------

صنف ګراف_Acyc_څنډه final : عام V3_ګراف_څنډه {
    VL_RTTI_IMPL(ګراف_Acyc_څنډه, V3_ګراف_څنډه)
    // userp() is always used to point to the head original graph edge
خاص:
    کارول Orig_څنډه_List = std::list<V3_ګراف_څنډه*>;  // List of orig edges, see also GraphAcyc's decl
    V3_ګراف_څنډه* origEdgep() ثابت {
        ثابت Orig_څنډه_List* ثابت oEListp = static_cast<Orig_څنډه_List*>(userp());
        UASSERT(oEListp, "No original edge associated with acyc edge " << هذا);
        أرجع (oEListp->front());
    }

عام:
    ګراف_Acyc_څنډه(V3_ګراف* graphp, V3_ګراف_راس* fromp, V3_ګراف_راس* سر, صحيح وزن_2,
                  منطقي cutable = خطأ)
        : V3_ګراف_څنډه{graphp, fromp, سر, وزن_2, cutable} {}
    ~ګراف_Acyc_څنډه() override = افتراضي;
    // yellow=we might still cut it, else oldEdge: yellowGreen=made uncutable, red=uncutable
    string dotColor() ثابت override { أرجع (cutable() ? "yellow" : origEdgep()->dotColor()); }
};

//--------------------------------------------------------------------

بنية ګراف_Acyc_څنډه_Cmp final {
    منطقي چلوونکی()(ثابت V3_ګراف_څنډه* lhsp, ثابت V3_ګراف_څنډه* rhsp) ثابت {
        إذا (lhsp->وزن_2() > rhsp->وزن_2()) أرجع ریښتیا;  // LHS goes first
        إذا (lhsp->وزن_2() < rhsp->وزن_2()) أرجع خطأ;  // RHS goes first
        أرجع خطأ;
    }
};

//--------------------------------------------------------------------

// CLASSES
صنف ګراف_Acyc final {
    کارول Orig_څنډه_List
        = std::list<V3_ګراف_څنډه*>;  // List of orig edges, see also GraphAcycEdge's decl
    // GRAPH USERS
    //  origGraph
    //    GraphVertex::user()   GraphAycVerted* New graph node
    //  m_breakGraph
    //    GraphEdge::user()     OrigEdgeList*   Old graph edges
    //    GraphVertex::user     bool            Detection of loops in simplifyDupIterate
    // MEMBERS
    V3_ګراف* ثابت m_origGraphp;  // Original graph
    V3_ګراف m_break_ګراف;  // Graph with only breakable edges represented
    ګراف_Acyc_راس::List m_work;  // List of vertices with optimization work left
    std::ویکټور<Orig_څنډه_List*> m_orig_څنډه_Delp;  // List of deletions to do when done
    ثابت V3_څنډه_Func_P
        m_orig_څنډه_Funcp;  // Function that says we follow this edge (in original graph)
    uint32_t m_ځای_پرځای_کړه_ګام = 0;  // Number that user() must be equal to to indicate processing

    // METHODS
    فراغ build_ګراف(V3_ګراف* origGraphp);
    فراغ build_ګراف_Iterate(V3_ګراف_راس* overtexp, ګراف_Acyc_راس* avertexp);
    فراغ simplify(منطقي allowCut);
    فراغ simplifyNone(ګراف_Acyc_راس* avertexp);
    فراغ simplifyOne(ګراف_Acyc_راس* avertexp);
    فراغ simplify_بهر(ګراف_Acyc_راس* avertexp);
    فراغ simplifyDup(ګراف_Acyc_راس* avertexp);
    فراغ cutBasic(ګراف_Acyc_راس* avertexp);
    فراغ cutBackward(ګراف_Acyc_راس* avertexp);
    فراغ deleteMarked();
    فراغ ځای_پرځای_کړه_2();
    فراغ ځای_پرځای_کړه_Try_څنډه(V3_ګراف_څنډه* edgep);
    منطقي ځای_پرځای_کړه_Iterate(ګراف_Acyc_راس* vertexp, uint32_t currentRank);

    منطقي orig_Follow_څنډه(V3_ګراف_څنډه* edgep) {
        أرجع (edgep->وزن_2() && (m_orig_څنډه_Funcp)(edgep));
    }
    فراغ څنډه_From_څنډه(V3_ګراف_څنډه* oldedgep, V3_ګراف_راس* fromp, V3_ګراف_راس* سر) {
        // Make new breakGraph edge, with old edge as a template
        ګراف_Acyc_څنډه* ثابت newEdgep = جديد ګراف_Acyc_څنډه{&m_break_ګراف, fromp, سر,
                                                          oldedgep->وزن_2(), oldedgep->cutable()};
        newEdgep->userp(oldedgep->userp());  // Keep pointer to OrigEdgeList
    }
    فراغ جمع_کړه_Orig_Edgep(V3_ګراف_څنډه* منزل_Edgep, V3_ګراف_څنډه* جمع_کړه_Edgep) {
        // Add addEdge (or it's list) to list of edges that break edge represents
        // Note addEdge may already have a bunch of similar linked edge representations.  Yuk.
        UASSERT(جمع_کړه_Edgep, "Adding nullptr");
        إذا (!منزل_Edgep->userp()) {
            Orig_څنډه_List* ثابت oep = جديد Orig_څنډه_List;
            m_orig_څنډه_Delp.ټېل_شا(oep);
            منزل_Edgep->userp(oep);
        }
        Orig_څنډه_List* ثابت oEListp = static_cast<Orig_څنډه_List*>(منزل_Edgep->userp());
        إذا (Orig_څنډه_List* ثابت جمع_کړه_Listp = static_cast<Orig_څنډه_List*>(جمع_کړه_Edgep->userp())) {
            لكل (ثابت خپلکاری& itr : *جمع_کړه_Listp) oEListp->ټېل_شا(itr);
            جمع_کړه_Listp->clear();  // Done with it
        } وإلا {
            oEListp->ټېل_شا(جمع_کړه_Edgep);
        }
    }
    فراغ cut_Orig_څنډه(V3_ګراف_څنډه* breakEdgep, ثابت حرف* why) {
        // From the break edge, cut edges in original graph it represents
        UINFO(8, why << " CUT " << breakEdgep->fromp());
        breakEdgep->cut();
        ثابت Orig_څنډه_List* ثابت oEListp = static_cast<Orig_څنډه_List*>(breakEdgep->userp());
        إذا (!oEListp) {
            v3fatalSrc("No original edge associated with cutting edge " << breakEdgep);
        }
        // The breakGraph edge may represent multiple real edges; cut them all
        لكل (ثابت خپلکاری& origEdgep : *oEListp) {
            origEdgep->cut();
            UINFO(8, "  " << why << "   " << origEdgep->fromp() << " ->" << origEdgep->سر());
        }
    }
    // Work Queue
    فراغ work_ټېل(V3_ګراف_راس* vertexp) {
        ګراف_Acyc_راس* ثابت avertexp = static_cast<ګراف_Acyc_راس*>(vertexp);
        // Add vertex to list of nodes needing further optimization trials
        إذا (!avertexp->m_onWorkList) {
            avertexp->m_onWorkList = ریښتیا;
            m_work.link_شا(avertexp);
        }
    }
    ګراف_Acyc_راس* workBeginp() { أرجع m_work.frontp(); }
    فراغ work_راوباسه() {
        ګراف_Acyc_راس* ثابت avertexp = workBeginp();
        avertexp->m_onWorkList = خطأ;
        m_work.unlink(avertexp);
    }

عام:
    // CONSTRUCTORS
    ګراف_Acyc(V3_ګراف* origGraphp, V3_څنډه_Func_P څنډه_Funcp)
        : m_origGraphp{origGraphp}
        , m_orig_څنډه_Funcp{څنډه_Funcp} {}
    ~ګراف_Acyc() {
        لكل (Orig_څنډه_List* ip : m_orig_څنډه_Delp) ړنګول ip;
        m_orig_څنډه_Delp.clear();
    }
    فراغ اصلي();
};

//--------------------------------------------------------------------

فراغ ګراف_Acyc::build_ګراف(V3_ګراف* origGraphp) {
    // Presumes the graph has been strongly ordered,
    // and thus there's a unique color if there are loops in this subgraph.

    // For each old node, make a new graph node for optimization
    origGraphp->userClearVertices();
    origGraphp->user_Clear_څنډې();
    لكل (V3_ګراف_راس& overtex : origGraphp->vertices()) {
        إذا (overtex.color()) {
            ګراف_Acyc_راس* ثابت avertexp = جديد ګراف_Acyc_راس{&m_break_ګراف, &overtex};
            overtex.userp(avertexp);  // Stash so can look up later
        }
    }

    // Build edges between logic vertices
    لكل (V3_ګراف_راس& overtex : origGraphp->vertices()) {
        إذا (overtex.color()) {
            ګراف_Acyc_راس* ثابت avertexp = static_cast<ګراف_Acyc_راس*>(overtex.userp());
            build_ګراف_Iterate(&overtex, avertexp);
        }
    }
}

فراغ ګراف_Acyc::build_ګراف_Iterate(V3_ګراف_راس* overtexp, ګراف_Acyc_راس* avertexp) {
    // Make new edges
    لكل (V3_ګراف_څنډه& څنډه : overtexp->بهر_څنډې()) {
        إذا (orig_Follow_څنډه(&څنډه)) {  // not cut
            ثابت V3_ګراف_راس* منزل_Vertexp = څنډه.سر();
            إذا (منزل_Vertexp->color()) {
                ګراف_Acyc_راس* ثابت منزل_AVertexp
                    = static_cast<ګراف_Acyc_راس*>(منزل_Vertexp->userp());
                // Replicate the old edge into the new graph
                // There may be multiple edges between same pairs of vertices
                V3_ګراف_څنډه* breakEdgep = جديد ګراف_Acyc_څنډه{&m_break_ګراف, avertexp, منزل_AVertexp,
                                                            څنډه.وزن_2(), څنډه.cutable()};
                جمع_کړه_Orig_Edgep(breakEdgep, &څنډه);  // So can find original edge
            }
        }
    }
}

فراغ ګراف_Acyc::simplify(منطقي allowCut) {
    // Add all nodes to list of work to do
    لكل (V3_ګراف_راس& راس : m_break_ګراف.vertices()) work_ټېل(&راس);
    // Optimize till everything finished
    طالما (ګراف_Acyc_راس* vertexp = workBeginp()) {
        work_راوباسه();
        simplifyNone(vertexp);
        simplifyOne(vertexp);
        simplify_بهر(vertexp);
        simplifyDup(vertexp);
        إذا (allowCut) {
            // The main algorithm works without these, though slower
            // So if changing the main algorithm, comment these out for a test run
            إذا (v3Global.opt.fAcycSimp()) {
                cutBasic(vertexp);
                cutBackward(vertexp);
            }
        }
    }
    deleteMarked();
}

فراغ ګراف_Acyc::deleteMarked() {
    // Delete nodes marked for removal
    لكل (V3_ګراف_راس* ثابت vtxp : m_break_ګراف.vertices().unlinkable()) {
        ګراف_Acyc_راس* ثابت avertexp = static_cast<ګراف_Acyc_راس*>(vtxp);
        إذا (avertexp->isDelete()) {
            VL_DO_DANGLING(avertexp->unlinkDelete(&m_break_ګراف), avertexp);
        }
    }
}

فراغ ګراف_Acyc::simplifyNone(ګراف_Acyc_راس* avertexp) {
    // Don't need any vertices with no inputs, There's no way they can have a loop.
    // Likewise, vertices with no outputs
    إذا (avertexp->isDelete()) أرجع;
    إذا (avertexp->دننه_تشه() || avertexp->بهر_تشه()) {
        UINFO(9, "  SimplifyNoneRemove " << avertexp);
        avertexp->سېټ_Delete();  // Mark so we won't delete it twice
        // Remove edges
        طالما (V3_ګراف_څنډه* ثابت edgep = avertexp->بهر_څنډې().frontp()) {
            work_ټېل(edgep->سر());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        طالما (V3_ګراف_څنډه* ثابت edgep = avertexp->دننه_څنډې().frontp()) {
            work_ټېل(edgep->fromp());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
}

فراغ ګراف_Acyc::simplifyOne(ګراف_Acyc_راس* avertexp) {
    // If a node has one input and one output, we can remove it and change the edges
    إذا (avertexp->isDelete()) أرجع;
    إذا (avertexp->دننه_Size1() && avertexp->بهر_Size1()) {
        V3_ګراف_څنډه* ثابت دننه_Edgep = avertexp->دننه_څنډې().frontp();
        V3_ګراف_څنډه* ثابت بهر_Edgep = avertexp->بهر_څنډې().frontp();
        V3_ګراف_راس* ثابت دننه_Vertexp = دننه_Edgep->fromp();
        V3_ګراف_راس* ثابت بهر_Vertexp = بهر_Edgep->سر();
        // The in and out may be the same node; we'll make a loop
        // The in OR out may be THIS node; we can't delete it then.
        إذا (دننه_Vertexp != avertexp && بهر_Vertexp != avertexp) {
            UINFO(9, "  SimplifyOneRemove " << avertexp);
            avertexp->سېټ_Delete();  // Mark so we won't delete it twice
            // Make a new edge connecting the two vertices directly
            // If both are breakable, we pick the one with less weight, else it's arbitrary
            // We can forget about the origEdge list for the "non-selected" set of edges,
            // as we need to break only one set or the other set of edges, not both.
            // (This is why we must give preference to the cutable set.)
            V3_ګراف_څنډه* ثابت templateEdgep
                = ((دننه_Edgep->cutable()
                    && (!بهر_Edgep->cutable() || دننه_Edgep->وزن_2() < بهر_Edgep->وزن_2()))
                       ? دننه_Edgep
                       : بهر_Edgep);
            // cppcheck-suppress leakReturnValNotUsed
            څنډه_From_څنډه(templateEdgep, دننه_Vertexp, بهر_Vertexp);
            // Remove old edge
            VL_DO_DANGLING(دننه_Edgep->unlinkDelete(), دننه_Edgep);
            VL_DO_DANGLING(بهر_Edgep->unlinkDelete(), بهر_Edgep);
            VL_DANGLING(templateEdgep);
            work_ټېل(دننه_Vertexp);
            work_ټېل(بهر_Vertexp);
        }
    }
}

فراغ ګراف_Acyc::simplify_بهر(ګراف_Acyc_راس* avertexp) {
    // If a node has one output that's not cutable, all its inputs can be reassigned
    // to the next node in the list
    إذا (avertexp->isDelete()) أرجع;
    إذا (avertexp->بهر_Size1()) {
        V3_ګراف_څنډه* ثابت بهر_Edgep = avertexp->بهر_څنډې().frontp();
        إذا (!بهر_Edgep->cutable()) {
            V3_ګراف_راس* بهر_Vertexp = بهر_Edgep->سر();
            UINFO(9, "  SimplifyOutRemove " << avertexp);
            avertexp->سېټ_Delete();  // Mark so we won't delete it twice
            لكل (V3_ګراف_څنډه* ثابت دننه_Edgep : avertexp->دننه_څنډې().unlinkable()) {
                V3_ګراف_راس* دننه_Vertexp = دننه_Edgep->fromp();
                إذا (دننه_Vertexp == avertexp) {
                    إذا (debug()) v3error("Non-cutable vertex=" << avertexp);  // LCOV_EXCL_LINE
                    v3error("Circular logic when ordering code (non-cutable edge loop)\n"
                            << m_origGraphp->reportLoops(  // calls OrderGraph::loopsVertexCb
                                   &V3_ګراف_څنډه::followNotCutable, avertexp->origVertexp()));
                    // Things are unlikely to end well at this point,
                    // but we'll try something to get to further errors...
                    دننه_Edgep->cutable(ریښتیا);
                    أرجع;
                }
                // Make a new edge connecting the two vertices directly
                // cppcheck-suppress leakReturnValNotUsed
                څنډه_From_څنډه(دننه_Edgep, دننه_Vertexp, بهر_Vertexp);
                // Remove old edge
                VL_DO_DANGLING(دننه_Edgep->unlinkDelete(), دننه_Edgep);
                work_ټېل(دننه_Vertexp);
            }
            VL_DO_DANGLING(بهر_Edgep->unlinkDelete(), بهر_Edgep);
            work_ټېل(بهر_Vertexp);
        }
    }
}

فراغ ګراف_Acyc::simplifyDup(ګراف_Acyc_راس* avertexp) {
    // Remove redundant edges
    إذا (avertexp->isDelete()) أرجع;
    // Clear marks
    لكل (V3_ګراف_څنډه& څنډه : avertexp->بهر_څنډې()) څنډه.سر()->userp(تش_اشاره);
    // Mark edges and detect duplications
    لكل (V3_ګراف_څنډه* ثابت edgep : avertexp->بهر_څنډې().unlinkable()) {
        V3_ګراف_راس* بهر_Vertexp = edgep->سر();
        V3_ګراف_څنډه* prevEdgep = static_cast<V3_ګراف_څنډه*>(بهر_Vertexp->userp());
        إذا (prevEdgep) {
            إذا (!prevEdgep->cutable()) {
                // !cutable duplicates prev !cutable: we can ignore it, redundant
                //  cutable duplicates prev !cutable: know it's not a relevant loop, ignore it
                UINFO(8, "    DelDupEdge " << avertexp << " -> " << edgep->سر());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            } وإلا إذا (!edgep->cutable()) {
                // !cutable duplicates prev  cutable: delete the earlier cutable
                UINFO(8, "    DelDupPrev " << avertexp << " -> " << prevEdgep->سر());
                VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                بهر_Vertexp->userp(edgep);
            } وإلا {
                //  cutable duplicates prev  cutable: combine weights
                UINFO(8, "    DelDupComb " << avertexp << " -> " << edgep->سر());
                prevEdgep->وزن_2(prevEdgep->وزن_2() + edgep->وزن_2());
                جمع_کړه_Orig_Edgep(prevEdgep, edgep);
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            work_ټېل(بهر_Vertexp);
            work_ټېل(avertexp);
        } وإلا {
            // No previous assignment
            بهر_Vertexp->userp(edgep);
        }
    }
}

فراغ ګراف_Acyc::cutBasic(ګراف_Acyc_راس* avertexp) {
    // Detect and cleanup any loops from node to itself
    إذا (avertexp->isDelete()) أرجع;
    لكل (V3_ګراف_څنډه* ثابت edgep : avertexp->بهر_څنډې().unlinkable()) {
        إذا (edgep->cutable() && edgep->سر() == avertexp) {
            cut_Orig_څنډه(edgep, "  Cut Basic");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_ټېل(avertexp);
        }
    }
}

فراغ ګراف_Acyc::cutBackward(ګراف_Acyc_راس* avertexp) {
    // If a cutable edge is from A->B, and there's a non-cutable edge B->A, then must cut!
    إذا (avertexp->isDelete()) أرجع;
    // Clear marks
    لكل (V3_ګراف_څنډه& څنډه : avertexp->بهر_څنډې()) څنډه.سر()->user(خطأ);
    لكل (V3_ګراف_څنډه& څنډه : avertexp->دننه_څنډې()) {
        إذا (!څنډه.cutable()) څنډه.fromp()->user(ریښتیا);
    }
    // Detect duplications
    لكل (V3_ګراف_څنډه* ثابت edgep : avertexp->بهر_څنډې().unlinkable()) {
        إذا (edgep->cutable() && edgep->سر()->user()) {
            cut_Orig_څنډه(edgep, "  Cut A->B->A");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_ټېل(avertexp);
        }
    }
}

فراغ ګراف_Acyc::ځای_پرځای_کړه_2() {
    // Input is m_breakGraph with ranks already assigned on non-breakable edges

    // Make a list of all cutable edges in the graph
    صحيح عدد_څنډې = 0;
    لكل (V3_ګراف_راس& راس : m_break_ګراف.vertices()) {
        لكل (V3_ګراف_څنډه& څنډه : راس.بهر_څنډې()) {
            إذا (څنډه.وزن_2() && څنډه.cutable()) ++عدد_څنډې;
        }
    }
    UINFO(4, "    Cutable edges = " << عدد_څنډې);

    std::ویکټور<V3_ګراف_څنډه*> څنډې;  // List of all edges to be processed
    // Make the vector properly sized right off the bat -- faster than reallocating
    څنډې.reserve(عدد_څنډې + 1);
    لكل (V3_ګراف_راس& راس : m_break_ګراف.vertices()) {
        راس.user(0);  // Clear in prep of next step
        لكل (V3_ګراف_څنډه& څنډه : راس.بهر_څنډې()) {
            إذا (څنډه.وزن_2() && څنډه.cutable()) څنډې.ټېل_شا(&څنډه);
        }
    }

    // Sort by weight, then by vertex (so that we completely process one vertex, when possible)
    stable_ترتیبول(څنډې.begin(), څنډې.پای_ټکی(), ګراف_Acyc_څنډه_Cmp());

    // Process each edge in weighted order
    m_ځای_پرځای_کړه_ګام = 10;
    لكل (V3_ګراف_څنډه* edgep : څنډې) ځای_پرځای_کړه_Try_څنډه(edgep);
}

فراغ ګراف_Acyc::ځای_پرځای_کړه_Try_څنډه(V3_ګراف_څنډه* edgep) {
    // Try to make this edge uncutable
    m_ځای_پرځای_کړه_ګام++;
    UINFO(8, "    PlaceEdge s" << m_ځای_پرځای_کړه_ګام << " w" << edgep->وزن_2() << " " << edgep->fromp());
    // Make the edge uncutable so we detect it in placement
    edgep->cutable(خطأ);
    // Vertex::m_user begin: number indicates this edge was completed
    // Try to assign ranks, presuming this edge is in place
    // If we come across user()==placestep, we've detected a loop and must back out
    ثابت منطقي loop
        = ځای_پرځای_کړه_Iterate(static_cast<ګراف_Acyc_راس*>(edgep->سر()), edgep->fromp()->rank() + 1);
    إذا (!loop) {
        // No loop, we can keep it as uncutable
        // Commit the new ranks we calculated
        // Just cleanup the list.  If this is slow, we can add another set of
        // user counters to avoid cleaning up the list.
        طالما (workBeginp()) work_راوباسه();
    } وإلا {
        // Adding this edge would cause a loop, kill it
        edgep->cutable(ریښتیا);  // So graph still looks pretty
        cut_Orig_څنډه(edgep, "  Cut loop");
        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        // Back out the ranks we calculated
        طالما (ګراف_Acyc_راس* vertexp = workBeginp()) {
            work_راوباسه();
            vertexp->rank(vertexp->m_storedRank);
        }
    }
}

منطقي ګراف_Acyc::ځای_پرځای_کړه_Iterate(ګراف_Acyc_راس* vertexp, uint32_t currentRank) {
    // Assign rank to each unvisited node
    //   rank() is the "committed rank" of the graph known without loops
    // If larger rank is found, assign it and loop back through
    // If we hit a back node make a list of all loops
    إذا (vertexp->rank() >= currentRank) أرجع خطأ;  // Already processed it
    إذا (vertexp->user() == m_ځای_پرځای_کړه_ګام) أرجع ریښتیا;  // Loop detected
    vertexp->user(m_ځای_پرځای_کړه_ګام);
    // Remember we're changing the rank of this node; might need to back out
    إذا (!vertexp->m_onWorkList) {
        vertexp->m_storedRank = vertexp->rank();
        work_ټېل(vertexp);
    }
    vertexp->rank(currentRank);
    // Follow all edges and increase their ranks
    لكل (V3_ګراف_څنډه& څنډه : vertexp->بهر_څنډې()) {
        إذا (څنډه.وزن_2() && !څنډه.cutable()) {
            إذا (ځای_پرځای_کړه_Iterate(static_cast<ګراف_Acyc_راس*>(څنډه.سر()), currentRank + 1)) {
                // We don't need to reset user(); we'll use a different placeStep for the next edge
                أرجع ریښتیا;  // Loop detected
            }
        }
    }
    vertexp->user(0);
    أرجع خطأ;
}

//----- Main algorithm entry point

فراغ ګراف_Acyc::اصلي() {
    m_break_ګراف.user_Clear_څنډې();

    // Color based on possible loops
    m_origGraphp->stronglyConnected(m_orig_څنډه_Funcp);

    // Make a new graph with vertices that have only a single vertex
    // for each group of old vertices that are interconnected with unbreakable
    // edges (and thus can't represent loops - if we did the unbreakable
    // marking right, anyways)
    build_ګراف(m_origGraphp);
    إذا (dump_ګراف_Level() >= 6) m_break_ګراف.dumpDotFilePrefixed("acyc_pre");

    // Perform simple optimizations before any cuttings
    simplify(خطأ);
    إذا (dump_ګراف_Level() >= 5) m_break_ګراف.dumpDotFilePrefixed("acyc_simp");

    UINFO(4, " Cutting trivial loops");
    simplify(ریښتیا);
    إذا (dump_ګراف_Level() >= 6) m_break_ګراف.dumpDotFilePrefixed("acyc_mid");

    UINFO(4, " Ranking");
    m_break_ګراف.rank(&V3_ګراف_څنډه::followNotCutable);
    إذا (dump_ګراف_Level() >= 6) m_break_ګراف.dumpDotFilePrefixed("acyc_rank");

    UINFO(4, " Placement");
    ځای_پرځای_کړه_2();
    إذا (dump_ګراف_Level() >= 6) m_break_ګراف.dumpDotFilePrefixed("acyc_place");

    UINFO(4, " Final Ranking");
    // Only needed to assert there are no loops in completed graph
    m_break_ګراف.rank(&V3_ګراف_څنډه::followAlwaysTrue);
    إذا (dump_ګراف_Level() >= 6) m_break_ګراف.dumpDotFilePrefixed("acyc_done");
}

فراغ V3_ګراف::acyclic(V3_څنډه_Func_P څنډه_Funcp) {
    UINFO(4, "Acyclic");
    ګراف_Acyc acyc{هذا, څنډه_Funcp};
    acyc.اصلي();
    UINFO(4, "Acyclic done");
}
