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

#تعریف_کن VL_MT_DISABLED_کد_UNIT 1

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

صنف گراف_Acyc_رأس final : عام V3_گراف_رأس {
    VL_RTTI_IMPL(گراف_Acyc_رأس, V3_گراف_رأس)
    // user() is used for various sub-algorithm pieces
    V3_گراف_رأس* ثابت m_origVertexp;  // Pointer to first vertex this represents
محمي:
    دوست صنف گراف_Acyc;
    V3ListLinks<گراف_Acyc_رأس> m_links;  // List links to store instances of this class
    uint32_t m_storedRank = 0;  // Rank held until commit to edge placement
    منطقي m_onWorkList = خطأ;  // True if already on list of work to do
    منطقي m_deleted = خطأ;  // True if deleted

خاص:
    V3ListLinks<گراف_Acyc_رأس>& links() { أرجع m_links; }

عام:
    // List type to store instances of this class
    استفاده List = V3List<گراف_Acyc_رأس, &گراف_Acyc_رأس::links>;

    گراف_Acyc_رأس(V3_گراف* graphp, V3_گراف_رأس* origVertexp)
        : V3_گراف_رأس{graphp}
        , m_origVertexp{origVertexp} {}
    ~گراف_Acyc_رأس() override = افتراضي;
    V3_گراف_رأس* origVertexp() ثابت { أرجع m_origVertexp; }
    فراغ مجموعه_Delete() { m_deleted = درست; }
    منطقي isDelete() ثابت { أرجع m_deleted; }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string name() ثابت override { أرجع m_origVertexp->name(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string dotColor() ثابت override { أرجع m_origVertexp->dotColor(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    File_سطر* fileline() ثابت override { أرجع m_origVertexp->fileline(); }
};

//--------------------------------------------------------------------

صنف گراف_Acyc_یال final : عام V3_گراف_یال {
    VL_RTTI_IMPL(گراف_Acyc_یال, V3_گراف_یال)
    // userp() is always used to point to the head original graph edge
خاص:
    استفاده Orig_یال_List = std::list<V3_گراف_یال*>;  // List of orig edges, see also GraphAcyc's decl
    V3_گراف_یال* origEdgep() ثابت {
        ثابت Orig_یال_List* ثابت oEListp = static_cast<Orig_یال_List*>(userp());
        UASSERT(oEListp, "No original edge associated with acyc edge " << هذا);
        أرجع (oEListp->front());
    }

عام:
    گراف_Acyc_یال(V3_گراف* graphp, V3_گراف_رأس* fromp, V3_گراف_رأس* بالاترین, صحيح وزن_2,
                  منطقي cutable = خطأ)
        : V3_گراف_یال{graphp, fromp, بالاترین, وزن_2, cutable} {}
    ~گراف_Acyc_یال() override = افتراضي;
    // yellow=we might still cut it, else oldEdge: yellowGreen=made uncutable, red=uncutable
    string dotColor() ثابت override { أرجع (cutable() ? "yellow" : origEdgep()->dotColor()); }
};

//--------------------------------------------------------------------

بنية گراف_Acyc_یال_Cmp final {
    منطقي عملگر()(ثابت V3_گراف_یال* lhsp, ثابت V3_گراف_یال* rhsp) ثابت {
        إذا (lhsp->وزن_2() > rhsp->وزن_2()) أرجع درست;  // LHS goes first
        إذا (lhsp->وزن_2() < rhsp->وزن_2()) أرجع خطأ;  // RHS goes first
        أرجع خطأ;
    }
};

//--------------------------------------------------------------------

// CLASSES
صنف گراف_Acyc final {
    استفاده Orig_یال_List
        = std::list<V3_گراف_یال*>;  // List of orig edges, see also GraphAcycEdge's decl
    // GRAPH USERS
    //  origGraph
    //    GraphVertex::user()   GraphAycVerted* New graph node
    //  m_breakGraph
    //    GraphEdge::user()     OrigEdgeList*   Old graph edges
    //    GraphVertex::user     bool            Detection of loops in simplifyDupIterate
    // MEMBERS
    V3_گراف* ثابت m_origGraphp;  // Original graph
    V3_گراف m_break_گراف;  // Graph with only breakable edges represented
    گراف_Acyc_رأس::List m_work;  // List of vertices with optimization work left
    std::وکتور<Orig_یال_List*> m_orig_یال_Delp;  // List of deletions to do when done
    ثابت V3_یال_Func_P
        m_orig_یال_Funcp;  // Function that says we follow this edge (in original graph)
    uint32_t m_قراربده_گام = 0;  // Number that user() must be equal to to indicate processing

    // METHODS
    فراغ build_گراف(V3_گراف* origGraphp);
    فراغ build_گراف_Iterate(V3_گراف_رأس* overtexp, گراف_Acyc_رأس* avertexp);
    فراغ simplify(منطقي allowCut);
    فراغ simplifyNone(گراف_Acyc_رأس* avertexp);
    فراغ simplifyOne(گراف_Acyc_رأس* avertexp);
    فراغ simplify_بیرون(گراف_Acyc_رأس* avertexp);
    فراغ simplifyDup(گراف_Acyc_رأس* avertexp);
    فراغ cutBasic(گراف_Acyc_رأس* avertexp);
    فراغ cutBackward(گراف_Acyc_رأس* avertexp);
    فراغ deleteMarked();
    فراغ قراربده_2();
    فراغ قراربده_Try_یال(V3_گراف_یال* edgep);
    منطقي قراربده_Iterate(گراف_Acyc_رأس* vertexp, uint32_t currentRank);

    منطقي orig_Follow_یال(V3_گراف_یال* edgep) {
        أرجع (edgep->وزن_2() && (m_orig_یال_Funcp)(edgep));
    }
    فراغ یال_From_یال(V3_گراف_یال* oldedgep, V3_گراف_رأس* fromp, V3_گراف_رأس* بالاترین) {
        // Make new breakGraph edge, with old edge as a template
        گراف_Acyc_یال* ثابت newEdgep = جديد گراف_Acyc_یال{&m_break_گراف, fromp, بالاترین,
                                                          oldedgep->وزن_2(), oldedgep->cutable()};
        newEdgep->userp(oldedgep->userp());  // Keep pointer to OrigEdgeList
    }
    فراغ جمع_کن_Orig_Edgep(V3_گراف_یال* مقصد_Edgep, V3_گراف_یال* جمع_کن_Edgep) {
        // Add addEdge (or it's list) to list of edges that break edge represents
        // Note addEdge may already have a bunch of similar linked edge representations.  Yuk.
        UASSERT(جمع_کن_Edgep, "Adding nullptr");
        إذا (!مقصد_Edgep->userp()) {
            Orig_یال_List* ثابت oep = جديد Orig_یال_List;
            m_orig_یال_Delp.افزودن_پشت(oep);
            مقصد_Edgep->userp(oep);
        }
        Orig_یال_List* ثابت oEListp = static_cast<Orig_یال_List*>(مقصد_Edgep->userp());
        إذا (Orig_یال_List* ثابت جمع_کن_Listp = static_cast<Orig_یال_List*>(جمع_کن_Edgep->userp())) {
            لكل (ثابت خودکار& itr : *جمع_کن_Listp) oEListp->افزودن_پشت(itr);
            جمع_کن_Listp->clear();  // Done with it
        } وإلا {
            oEListp->افزودن_پشت(جمع_کن_Edgep);
        }
    }
    فراغ cut_Orig_یال(V3_گراف_یال* breakEdgep, ثابت حرف* why) {
        // From the break edge, cut edges in original graph it represents
        UINFO(8, why << " CUT " << breakEdgep->fromp());
        breakEdgep->cut();
        ثابت Orig_یال_List* ثابت oEListp = static_cast<Orig_یال_List*>(breakEdgep->userp());
        إذا (!oEListp) {
            v3fatalSrc("No original edge associated with cutting edge " << breakEdgep);
        }
        // The breakGraph edge may represent multiple real edges; cut them all
        لكل (ثابت خودکار& origEdgep : *oEListp) {
            origEdgep->cut();
            UINFO(8, "  " << why << "   " << origEdgep->fromp() << " ->" << origEdgep->بالاترین());
        }
    }
    // Work Queue
    فراغ work_افزودن(V3_گراف_رأس* vertexp) {
        گراف_Acyc_رأس* ثابت avertexp = static_cast<گراف_Acyc_رأس*>(vertexp);
        // Add vertex to list of nodes needing further optimization trials
        إذا (!avertexp->m_onWorkList) {
            avertexp->m_onWorkList = درست;
            m_work.link_پشت(avertexp);
        }
    }
    گراف_Acyc_رأس* workBeginp() { أرجع m_work.frontp(); }
    فراغ work_برداشت() {
        گراف_Acyc_رأس* ثابت avertexp = workBeginp();
        avertexp->m_onWorkList = خطأ;
        m_work.unlink(avertexp);
    }

عام:
    // CONSTRUCTORS
    گراف_Acyc(V3_گراف* origGraphp, V3_یال_Func_P یال_Funcp)
        : m_origGraphp{origGraphp}
        , m_orig_یال_Funcp{یال_Funcp} {}
    ~گراف_Acyc() {
        لكل (Orig_یال_List* ip : m_orig_یال_Delp) حذف ip;
        m_orig_یال_Delp.clear();
    }
    فراغ اصلی();
};

//--------------------------------------------------------------------

فراغ گراف_Acyc::build_گراف(V3_گراف* origGraphp) {
    // Presumes the graph has been strongly ordered,
    // and thus there's a unique color if there are loops in this subgraph.

    // For each old node, make a new graph node for optimization
    origGraphp->userClearVertices();
    origGraphp->user_Clear_یالها();
    لكل (V3_گراف_رأس& overtex : origGraphp->vertices()) {
        إذا (overtex.color()) {
            گراف_Acyc_رأس* ثابت avertexp = جديد گراف_Acyc_رأس{&m_break_گراف, &overtex};
            overtex.userp(avertexp);  // Stash so can look up later
        }
    }

    // Build edges between logic vertices
    لكل (V3_گراف_رأس& overtex : origGraphp->vertices()) {
        إذا (overtex.color()) {
            گراف_Acyc_رأس* ثابت avertexp = static_cast<گراف_Acyc_رأس*>(overtex.userp());
            build_گراف_Iterate(&overtex, avertexp);
        }
    }
}

فراغ گراف_Acyc::build_گراف_Iterate(V3_گراف_رأس* overtexp, گراف_Acyc_رأس* avertexp) {
    // Make new edges
    لكل (V3_گراف_یال& یال : overtexp->بیرون_یالها()) {
        إذا (orig_Follow_یال(&یال)) {  // not cut
            ثابت V3_گراف_رأس* مقصد_Vertexp = یال.بالاترین();
            إذا (مقصد_Vertexp->color()) {
                گراف_Acyc_رأس* ثابت مقصد_AVertexp
                    = static_cast<گراف_Acyc_رأس*>(مقصد_Vertexp->userp());
                // Replicate the old edge into the new graph
                // There may be multiple edges between same pairs of vertices
                V3_گراف_یال* breakEdgep = جديد گراف_Acyc_یال{&m_break_گراف, avertexp, مقصد_AVertexp,
                                                            یال.وزن_2(), یال.cutable()};
                جمع_کن_Orig_Edgep(breakEdgep, &یال);  // So can find original edge
            }
        }
    }
}

فراغ گراف_Acyc::simplify(منطقي allowCut) {
    // Add all nodes to list of work to do
    لكل (V3_گراف_رأس& رأس : m_break_گراف.vertices()) work_افزودن(&رأس);
    // Optimize till everything finished
    طالما (گراف_Acyc_رأس* vertexp = workBeginp()) {
        work_برداشت();
        simplifyNone(vertexp);
        simplifyOne(vertexp);
        simplify_بیرون(vertexp);
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

فراغ گراف_Acyc::deleteMarked() {
    // Delete nodes marked for removal
    لكل (V3_گراف_رأس* ثابت vtxp : m_break_گراف.vertices().unlinkable()) {
        گراف_Acyc_رأس* ثابت avertexp = static_cast<گراف_Acyc_رأس*>(vtxp);
        إذا (avertexp->isDelete()) {
            VL_DO_DANGLING(avertexp->unlinkDelete(&m_break_گراف), avertexp);
        }
    }
}

فراغ گراف_Acyc::simplifyNone(گراف_Acyc_رأس* avertexp) {
    // Don't need any vertices with no inputs, There's no way they can have a loop.
    // Likewise, vertices with no outputs
    إذا (avertexp->isDelete()) أرجع;
    إذا (avertexp->درون_خالی() || avertexp->بیرون_خالی()) {
        UINFO(9, "  SimplifyNoneRemove " << avertexp);
        avertexp->مجموعه_Delete();  // Mark so we won't delete it twice
        // Remove edges
        طالما (V3_گراف_یال* ثابت edgep = avertexp->بیرون_یالها().frontp()) {
            work_افزودن(edgep->بالاترین());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        طالما (V3_گراف_یال* ثابت edgep = avertexp->درون_یالها().frontp()) {
            work_افزودن(edgep->fromp());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
}

فراغ گراف_Acyc::simplifyOne(گراف_Acyc_رأس* avertexp) {
    // If a node has one input and one output, we can remove it and change the edges
    إذا (avertexp->isDelete()) أرجع;
    إذا (avertexp->درون_Size1() && avertexp->بیرون_Size1()) {
        V3_گراف_یال* ثابت درون_Edgep = avertexp->درون_یالها().frontp();
        V3_گراف_یال* ثابت بیرون_Edgep = avertexp->بیرون_یالها().frontp();
        V3_گراف_رأس* ثابت درون_Vertexp = درون_Edgep->fromp();
        V3_گراف_رأس* ثابت بیرون_Vertexp = بیرون_Edgep->بالاترین();
        // The in and out may be the same node; we'll make a loop
        // The in OR out may be THIS node; we can't delete it then.
        إذا (درون_Vertexp != avertexp && بیرون_Vertexp != avertexp) {
            UINFO(9, "  SimplifyOneRemove " << avertexp);
            avertexp->مجموعه_Delete();  // Mark so we won't delete it twice
            // Make a new edge connecting the two vertices directly
            // If both are breakable, we pick the one with less weight, else it's arbitrary
            // We can forget about the origEdge list for the "non-selected" set of edges,
            // as we need to break only one set or the other set of edges, not both.
            // (This is why we must give preference to the cutable set.)
            V3_گراف_یال* ثابت templateEdgep
                = ((درون_Edgep->cutable()
                    && (!بیرون_Edgep->cutable() || درون_Edgep->وزن_2() < بیرون_Edgep->وزن_2()))
                       ? درون_Edgep
                       : بیرون_Edgep);
            // cppcheck-suppress leakReturnValNotUsed
            یال_From_یال(templateEdgep, درون_Vertexp, بیرون_Vertexp);
            // Remove old edge
            VL_DO_DANGLING(درون_Edgep->unlinkDelete(), درون_Edgep);
            VL_DO_DANGLING(بیرون_Edgep->unlinkDelete(), بیرون_Edgep);
            VL_DANGLING(templateEdgep);
            work_افزودن(درون_Vertexp);
            work_افزودن(بیرون_Vertexp);
        }
    }
}

فراغ گراف_Acyc::simplify_بیرون(گراف_Acyc_رأس* avertexp) {
    // If a node has one output that's not cutable, all its inputs can be reassigned
    // to the next node in the list
    إذا (avertexp->isDelete()) أرجع;
    إذا (avertexp->بیرون_Size1()) {
        V3_گراف_یال* ثابت بیرون_Edgep = avertexp->بیرون_یالها().frontp();
        إذا (!بیرون_Edgep->cutable()) {
            V3_گراف_رأس* بیرون_Vertexp = بیرون_Edgep->بالاترین();
            UINFO(9, "  SimplifyOutRemove " << avertexp);
            avertexp->مجموعه_Delete();  // Mark so we won't delete it twice
            لكل (V3_گراف_یال* ثابت درون_Edgep : avertexp->درون_یالها().unlinkable()) {
                V3_گراف_رأس* درون_Vertexp = درون_Edgep->fromp();
                إذا (درون_Vertexp == avertexp) {
                    إذا (debug()) v3error("Non-cutable vertex=" << avertexp);  // LCOV_EXCL_LINE
                    v3error("Circular logic when ordering code (non-cutable edge loop)\n"
                            << m_origGraphp->reportLoops(  // calls OrderGraph::loopsVertexCb
                                   &V3_گراف_یال::followNotCutable, avertexp->origVertexp()));
                    // Things are unlikely to end well at this point,
                    // but we'll try something to get to further errors...
                    درون_Edgep->cutable(درست);
                    أرجع;
                }
                // Make a new edge connecting the two vertices directly
                // cppcheck-suppress leakReturnValNotUsed
                یال_From_یال(درون_Edgep, درون_Vertexp, بیرون_Vertexp);
                // Remove old edge
                VL_DO_DANGLING(درون_Edgep->unlinkDelete(), درون_Edgep);
                work_افزودن(درون_Vertexp);
            }
            VL_DO_DANGLING(بیرون_Edgep->unlinkDelete(), بیرون_Edgep);
            work_افزودن(بیرون_Vertexp);
        }
    }
}

فراغ گراف_Acyc::simplifyDup(گراف_Acyc_رأس* avertexp) {
    // Remove redundant edges
    إذا (avertexp->isDelete()) أرجع;
    // Clear marks
    لكل (V3_گراف_یال& یال : avertexp->بیرون_یالها()) یال.بالاترین()->userp(اشاره_تهی);
    // Mark edges and detect duplications
    لكل (V3_گراف_یال* ثابت edgep : avertexp->بیرون_یالها().unlinkable()) {
        V3_گراف_رأس* بیرون_Vertexp = edgep->بالاترین();
        V3_گراف_یال* prevEdgep = static_cast<V3_گراف_یال*>(بیرون_Vertexp->userp());
        إذا (prevEdgep) {
            إذا (!prevEdgep->cutable()) {
                // !cutable duplicates prev !cutable: we can ignore it, redundant
                //  cutable duplicates prev !cutable: know it's not a relevant loop, ignore it
                UINFO(8, "    DelDupEdge " << avertexp << " -> " << edgep->بالاترین());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            } وإلا إذا (!edgep->cutable()) {
                // !cutable duplicates prev  cutable: delete the earlier cutable
                UINFO(8, "    DelDupPrev " << avertexp << " -> " << prevEdgep->بالاترین());
                VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                بیرون_Vertexp->userp(edgep);
            } وإلا {
                //  cutable duplicates prev  cutable: combine weights
                UINFO(8, "    DelDupComb " << avertexp << " -> " << edgep->بالاترین());
                prevEdgep->وزن_2(prevEdgep->وزن_2() + edgep->وزن_2());
                جمع_کن_Orig_Edgep(prevEdgep, edgep);
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            work_افزودن(بیرون_Vertexp);
            work_افزودن(avertexp);
        } وإلا {
            // No previous assignment
            بیرون_Vertexp->userp(edgep);
        }
    }
}

فراغ گراف_Acyc::cutBasic(گراف_Acyc_رأس* avertexp) {
    // Detect and cleanup any loops from node to itself
    إذا (avertexp->isDelete()) أرجع;
    لكل (V3_گراف_یال* ثابت edgep : avertexp->بیرون_یالها().unlinkable()) {
        إذا (edgep->cutable() && edgep->بالاترین() == avertexp) {
            cut_Orig_یال(edgep, "  Cut Basic");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_افزودن(avertexp);
        }
    }
}

فراغ گراف_Acyc::cutBackward(گراف_Acyc_رأس* avertexp) {
    // If a cutable edge is from A->B, and there's a non-cutable edge B->A, then must cut!
    إذا (avertexp->isDelete()) أرجع;
    // Clear marks
    لكل (V3_گراف_یال& یال : avertexp->بیرون_یالها()) یال.بالاترین()->user(خطأ);
    لكل (V3_گراف_یال& یال : avertexp->درون_یالها()) {
        إذا (!یال.cutable()) یال.fromp()->user(درست);
    }
    // Detect duplications
    لكل (V3_گراف_یال* ثابت edgep : avertexp->بیرون_یالها().unlinkable()) {
        إذا (edgep->cutable() && edgep->بالاترین()->user()) {
            cut_Orig_یال(edgep, "  Cut A->B->A");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_افزودن(avertexp);
        }
    }
}

فراغ گراف_Acyc::قراربده_2() {
    // Input is m_breakGraph with ranks already assigned on non-breakable edges

    // Make a list of all cutable edges in the graph
    صحيح رقم_یالها = 0;
    لكل (V3_گراف_رأس& رأس : m_break_گراف.vertices()) {
        لكل (V3_گراف_یال& یال : رأس.بیرون_یالها()) {
            إذا (یال.وزن_2() && یال.cutable()) ++رقم_یالها;
        }
    }
    UINFO(4, "    Cutable edges = " << رقم_یالها);

    std::وکتور<V3_گراف_یال*> یالها;  // List of all edges to be processed
    // Make the vector properly sized right off the bat -- faster than reallocating
    یالها.reserve(رقم_یالها + 1);
    لكل (V3_گراف_رأس& رأس : m_break_گراف.vertices()) {
        رأس.user(0);  // Clear in prep of next step
        لكل (V3_گراف_یال& یال : رأس.بیرون_یالها()) {
            إذا (یال.وزن_2() && یال.cutable()) یالها.افزودن_پشت(&یال);
        }
    }

    // Sort by weight, then by vertex (so that we completely process one vertex, when possible)
    stable_مرتب_سازی(یالها.begin(), یالها.انتها(), گراف_Acyc_یال_Cmp());

    // Process each edge in weighted order
    m_قراربده_گام = 10;
    لكل (V3_گراف_یال* edgep : یالها) قراربده_Try_یال(edgep);
}

فراغ گراف_Acyc::قراربده_Try_یال(V3_گراف_یال* edgep) {
    // Try to make this edge uncutable
    m_قراربده_گام++;
    UINFO(8, "    PlaceEdge s" << m_قراربده_گام << " w" << edgep->وزن_2() << " " << edgep->fromp());
    // Make the edge uncutable so we detect it in placement
    edgep->cutable(خطأ);
    // Vertex::m_user begin: number indicates this edge was completed
    // Try to assign ranks, presuming this edge is in place
    // If we come across user()==placestep, we've detected a loop and must back out
    ثابت منطقي loop
        = قراربده_Iterate(static_cast<گراف_Acyc_رأس*>(edgep->بالاترین()), edgep->fromp()->rank() + 1);
    إذا (!loop) {
        // No loop, we can keep it as uncutable
        // Commit the new ranks we calculated
        // Just cleanup the list.  If this is slow, we can add another set of
        // user counters to avoid cleaning up the list.
        طالما (workBeginp()) work_برداشت();
    } وإلا {
        // Adding this edge would cause a loop, kill it
        edgep->cutable(درست);  // So graph still looks pretty
        cut_Orig_یال(edgep, "  Cut loop");
        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        // Back out the ranks we calculated
        طالما (گراف_Acyc_رأس* vertexp = workBeginp()) {
            work_برداشت();
            vertexp->rank(vertexp->m_storedRank);
        }
    }
}

منطقي گراف_Acyc::قراربده_Iterate(گراف_Acyc_رأس* vertexp, uint32_t currentRank) {
    // Assign rank to each unvisited node
    //   rank() is the "committed rank" of the graph known without loops
    // If larger rank is found, assign it and loop back through
    // If we hit a back node make a list of all loops
    إذا (vertexp->rank() >= currentRank) أرجع خطأ;  // Already processed it
    إذا (vertexp->user() == m_قراربده_گام) أرجع درست;  // Loop detected
    vertexp->user(m_قراربده_گام);
    // Remember we're changing the rank of this node; might need to back out
    إذا (!vertexp->m_onWorkList) {
        vertexp->m_storedRank = vertexp->rank();
        work_افزودن(vertexp);
    }
    vertexp->rank(currentRank);
    // Follow all edges and increase their ranks
    لكل (V3_گراف_یال& یال : vertexp->بیرون_یالها()) {
        إذا (یال.وزن_2() && !یال.cutable()) {
            إذا (قراربده_Iterate(static_cast<گراف_Acyc_رأس*>(یال.بالاترین()), currentRank + 1)) {
                // We don't need to reset user(); we'll use a different placeStep for the next edge
                أرجع درست;  // Loop detected
            }
        }
    }
    vertexp->user(0);
    أرجع خطأ;
}

//----- Main algorithm entry point

فراغ گراف_Acyc::اصلی() {
    m_break_گراف.user_Clear_یالها();

    // Color based on possible loops
    m_origGraphp->stronglyConnected(m_orig_یال_Funcp);

    // Make a new graph with vertices that have only a single vertex
    // for each group of old vertices that are interconnected with unbreakable
    // edges (and thus can't represent loops - if we did the unbreakable
    // marking right, anyways)
    build_گراف(m_origGraphp);
    إذا (dump_گراف_Level() >= 6) m_break_گراف.dumpDotFilePrefixed("acyc_pre");

    // Perform simple optimizations before any cuttings
    simplify(خطأ);
    إذا (dump_گراف_Level() >= 5) m_break_گراف.dumpDotFilePrefixed("acyc_simp");

    UINFO(4, " Cutting trivial loops");
    simplify(درست);
    إذا (dump_گراف_Level() >= 6) m_break_گراف.dumpDotFilePrefixed("acyc_mid");

    UINFO(4, " Ranking");
    m_break_گراف.rank(&V3_گراف_یال::followNotCutable);
    إذا (dump_گراف_Level() >= 6) m_break_گراف.dumpDotFilePrefixed("acyc_rank");

    UINFO(4, " Placement");
    قراربده_2();
    إذا (dump_گراف_Level() >= 6) m_break_گراف.dumpDotFilePrefixed("acyc_place");

    UINFO(4, " Final Ranking");
    // Only needed to assert there are no loops in completed graph
    m_break_گراف.rank(&V3_گراف_یال::followAlwaysTrue);
    إذا (dump_گراف_Level() >= 6) m_break_گراف.dumpDotFilePrefixed("acyc_done");
}

فراغ V3_گراف::acyclic(V3_یال_Func_P یال_Funcp) {
    UINFO(4, "Acyclic");
    گراف_Acyc acyc{هذا, یال_Funcp};
    acyc.اصلی();
    UINFO(4, "Acyclic done");
}
