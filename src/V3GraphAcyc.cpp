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

#وصف_ڪريو VL_MT_DISABLED_ڪوڊ_UNIT 1

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

صنف گراف_Acyc_راس final : عام V3_گراف_راس {
    VL_RTTI_IMPL(گراف_Acyc_راس, V3_گراف_راس)
    // user() is used for various sub-algorithm pieces
    V3_گراف_راس* ثابت m_origVertexp;  // Pointer to first vertex this represents
محمي:
    دوست صنف گراف_Acyc;
    V3ListLinks<گراف_Acyc_راس> m_links;  // List links to store instances of this class
    uint32_t m_storedRank = 0;  // Rank held until commit to edge placement
    منطقي m_onWorkList = خطأ;  // True if already on list of work to do
    منطقي m_deleted = خطأ;  // True if deleted

خاص:
    V3ListLinks<گراف_Acyc_راس>& links() { أرجع m_links; }

عام:
    // List type to store instances of this class
    استعمال List = V3List<گراف_Acyc_راس, &گراف_Acyc_راس::links>;

    گراف_Acyc_راس(V3_گراف* graphp, V3_گراف_راس* origVertexp)
        : V3_گراف_راس{graphp}
        , m_origVertexp{origVertexp} {}
    ~گراف_Acyc_راس() override = افتراضي;
    V3_گراف_راس* origVertexp() ثابت { أرجع m_origVertexp; }
    فراغ سيٽ_Delete() { m_deleted = سچ; }
    منطقي isDelete() ثابت { أرجع m_deleted; }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string name() ثابت override { أرجع m_origVertexp->name(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string dotColor() ثابت override { أرجع m_origVertexp->dotColor(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    File_سٽ* fileline() ثابت override { أرجع m_origVertexp->fileline(); }
};

//--------------------------------------------------------------------

صنف گراف_Acyc_ڪنارو final : عام V3_گراف_ڪنارو {
    VL_RTTI_IMPL(گراف_Acyc_ڪنارو, V3_گراف_ڪنارو)
    // userp() is always used to point to the head original graph edge
خاص:
    استعمال Orig_ڪنارو_List = std::list<V3_گراف_ڪنارو*>;  // List of orig edges, see also GraphAcyc's decl
    V3_گراف_ڪنارو* origEdgep() ثابت {
        ثابت Orig_ڪنارو_List* ثابت oEListp = static_cast<Orig_ڪنارو_List*>(userp());
        UASSERT(oEListp, "No original edge associated with acyc edge " << هذا);
        أرجع (oEListp->front());
    }

عام:
    گراف_Acyc_ڪنارو(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٽي, صحيح وزن_2,
                  منطقي cutable = خطأ)
        : V3_گراف_ڪنارو{graphp, fromp, چوٽي, وزن_2, cutable} {}
    ~گراف_Acyc_ڪنارو() override = افتراضي;
    // yellow=we might still cut it, else oldEdge: yellowGreen=made uncutable, red=uncutable
    string dotColor() ثابت override { أرجع (cutable() ? "yellow" : origEdgep()->dotColor()); }
};

//--------------------------------------------------------------------

بنية گراف_Acyc_ڪنارو_Cmp final {
    منطقي عامل()(ثابت V3_گراف_ڪنارو* lhsp, ثابت V3_گراف_ڪنارو* rhsp) ثابت {
        إذا (lhsp->وزن_2() > rhsp->وزن_2()) أرجع سچ;  // LHS goes first
        إذا (lhsp->وزن_2() < rhsp->وزن_2()) أرجع خطأ;  // RHS goes first
        أرجع خطأ;
    }
};

//--------------------------------------------------------------------

// CLASSES
صنف گراف_Acyc final {
    استعمال Orig_ڪنارو_List
        = std::list<V3_گراف_ڪنارو*>;  // List of orig edges, see also GraphAcycEdge's decl
    // GRAPH USERS
    //  origGraph
    //    GraphVertex::user()   GraphAycVerted* New graph node
    //  m_breakGraph
    //    GraphEdge::user()     OrigEdgeList*   Old graph edges
    //    GraphVertex::user     bool            Detection of loops in simplifyDupIterate
    // MEMBERS
    V3_گراف* ثابت m_origGraphp;  // Original graph
    V3_گراف m_break_گراف;  // Graph with only breakable edges represented
    گراف_Acyc_راس::List m_work;  // List of vertices with optimization work left
    std::ويڪٽر<Orig_ڪنارو_List*> m_orig_ڪنارو_Delp;  // List of deletions to do when done
    ثابت V3_ڪنارو_Func_P
        m_orig_ڪنارو_Funcp;  // Function that says we follow this edge (in original graph)
    uint32_t m_رکو_قدم = 0;  // Number that user() must be equal to to indicate processing

    // METHODS
    فراغ build_گراف(V3_گراف* origGraphp);
    فراغ build_گراف_Iterate(V3_گراف_راس* overtexp, گراف_Acyc_راس* avertexp);
    فراغ simplify(منطقي allowCut);
    فراغ simplifyNone(گراف_Acyc_راس* avertexp);
    فراغ simplifyOne(گراف_Acyc_راس* avertexp);
    فراغ simplify_ٻاهريون(گراف_Acyc_راس* avertexp);
    فراغ simplifyDup(گراف_Acyc_راس* avertexp);
    فراغ cutBasic(گراف_Acyc_راس* avertexp);
    فراغ cutBackward(گراف_Acyc_راس* avertexp);
    فراغ deleteMarked();
    فراغ رکو_2();
    فراغ رکو_Try_ڪنارو(V3_گراف_ڪنارو* edgep);
    منطقي رکو_Iterate(گراف_Acyc_راس* vertexp, uint32_t currentRank);

    منطقي orig_Follow_ڪنارو(V3_گراف_ڪنارو* edgep) {
        أرجع (edgep->وزن_2() && (m_orig_ڪنارو_Funcp)(edgep));
    }
    فراغ ڪنارو_From_ڪنارو(V3_گراف_ڪنارو* oldedgep, V3_گراف_راس* fromp, V3_گراف_راس* چوٽي) {
        // Make new breakGraph edge, with old edge as a template
        گراف_Acyc_ڪنارو* ثابت newEdgep = جديد گراف_Acyc_ڪنارو{&m_break_گراف, fromp, چوٽي,
                                                          oldedgep->وزن_2(), oldedgep->cutable()};
        newEdgep->userp(oldedgep->userp());  // Keep pointer to OrigEdgeList
    }
    فراغ جوڙيو_Orig_Edgep(V3_گراف_ڪنارو* منزل_Edgep, V3_گراف_ڪنارو* جوڙيو_Edgep) {
        // Add addEdge (or it's list) to list of edges that break edge represents
        // Note addEdge may already have a bunch of similar linked edge representations.  Yuk.
        UASSERT(جوڙيو_Edgep, "Adding nullptr");
        إذا (!منزل_Edgep->userp()) {
            Orig_ڪنارو_List* ثابت oep = جديد Orig_ڪنارو_List;
            m_orig_ڪنارو_Delp.ڌڪو_پٺ(oep);
            منزل_Edgep->userp(oep);
        }
        Orig_ڪنارو_List* ثابت oEListp = static_cast<Orig_ڪنارو_List*>(منزل_Edgep->userp());
        إذا (Orig_ڪنارو_List* ثابت جوڙيو_Listp = static_cast<Orig_ڪنارو_List*>(جوڙيو_Edgep->userp())) {
            لكل (ثابت خودڪار& itr : *جوڙيو_Listp) oEListp->ڌڪو_پٺ(itr);
            جوڙيو_Listp->clear();  // Done with it
        } وإلا {
            oEListp->ڌڪو_پٺ(جوڙيو_Edgep);
        }
    }
    فراغ cut_Orig_ڪنارو(V3_گراف_ڪنارو* breakEdgep, ثابت حرف* why) {
        // From the break edge, cut edges in original graph it represents
        UINFO(8, why << " CUT " << breakEdgep->fromp());
        breakEdgep->cut();
        ثابت Orig_ڪنارو_List* ثابت oEListp = static_cast<Orig_ڪنارو_List*>(breakEdgep->userp());
        إذا (!oEListp) {
            v3fatalSrc("No original edge associated with cutting edge " << breakEdgep);
        }
        // The breakGraph edge may represent multiple real edges; cut them all
        لكل (ثابت خودڪار& origEdgep : *oEListp) {
            origEdgep->cut();
            UINFO(8, "  " << why << "   " << origEdgep->fromp() << " ->" << origEdgep->چوٽي());
        }
    }
    // Work Queue
    فراغ work_ڌڪو(V3_گراف_راس* vertexp) {
        گراف_Acyc_راس* ثابت avertexp = static_cast<گراف_Acyc_راس*>(vertexp);
        // Add vertex to list of nodes needing further optimization trials
        إذا (!avertexp->m_onWorkList) {
            avertexp->m_onWorkList = سچ;
            m_work.link_پٺ(avertexp);
        }
    }
    گراف_Acyc_راس* workBeginp() { أرجع m_work.frontp(); }
    فراغ work_ڪڍو() {
        گراف_Acyc_راس* ثابت avertexp = workBeginp();
        avertexp->m_onWorkList = خطأ;
        m_work.unlink(avertexp);
    }

عام:
    // CONSTRUCTORS
    گراف_Acyc(V3_گراف* origGraphp, V3_ڪنارو_Func_P ڪنارو_Funcp)
        : m_origGraphp{origGraphp}
        , m_orig_ڪنارو_Funcp{ڪنارو_Funcp} {}
    ~گراف_Acyc() {
        لكل (Orig_ڪنارو_List* ip : m_orig_ڪنارو_Delp) ميٽيو ip;
        m_orig_ڪنارو_Delp.clear();
    }
    فراغ مکيه();
};

//--------------------------------------------------------------------

فراغ گراف_Acyc::build_گراف(V3_گراف* origGraphp) {
    // Presumes the graph has been strongly ordered,
    // and thus there's a unique color if there are loops in this subgraph.

    // For each old node, make a new graph node for optimization
    origGraphp->userClearVertices();
    origGraphp->user_Clear_ڪنارا();
    لكل (V3_گراف_راس& overtex : origGraphp->vertices()) {
        إذا (overtex.color()) {
            گراف_Acyc_راس* ثابت avertexp = جديد گراف_Acyc_راس{&m_break_گراف, &overtex};
            overtex.userp(avertexp);  // Stash so can look up later
        }
    }

    // Build edges between logic vertices
    لكل (V3_گراف_راس& overtex : origGraphp->vertices()) {
        إذا (overtex.color()) {
            گراف_Acyc_راس* ثابت avertexp = static_cast<گراف_Acyc_راس*>(overtex.userp());
            build_گراف_Iterate(&overtex, avertexp);
        }
    }
}

فراغ گراف_Acyc::build_گراف_Iterate(V3_گراف_راس* overtexp, گراف_Acyc_راس* avertexp) {
    // Make new edges
    لكل (V3_گراف_ڪنارو& ڪنارو : overtexp->ٻاهريون_ڪنارا()) {
        إذا (orig_Follow_ڪنارو(&ڪنارو)) {  // not cut
            ثابت V3_گراف_راس* منزل_Vertexp = ڪنارو.چوٽي();
            إذا (منزل_Vertexp->color()) {
                گراف_Acyc_راس* ثابت منزل_AVertexp
                    = static_cast<گراف_Acyc_راس*>(منزل_Vertexp->userp());
                // Replicate the old edge into the new graph
                // There may be multiple edges between same pairs of vertices
                V3_گراف_ڪنارو* breakEdgep = جديد گراف_Acyc_ڪنارو{&m_break_گراف, avertexp, منزل_AVertexp,
                                                            ڪنارو.وزن_2(), ڪنارو.cutable()};
                جوڙيو_Orig_Edgep(breakEdgep, &ڪنارو);  // So can find original edge
            }
        }
    }
}

فراغ گراف_Acyc::simplify(منطقي allowCut) {
    // Add all nodes to list of work to do
    لكل (V3_گراف_راس& راس : m_break_گراف.vertices()) work_ڌڪو(&راس);
    // Optimize till everything finished
    طالما (گراف_Acyc_راس* vertexp = workBeginp()) {
        work_ڪڍو();
        simplifyNone(vertexp);
        simplifyOne(vertexp);
        simplify_ٻاهريون(vertexp);
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
    لكل (V3_گراف_راس* ثابت vtxp : m_break_گراف.vertices().unlinkable()) {
        گراف_Acyc_راس* ثابت avertexp = static_cast<گراف_Acyc_راس*>(vtxp);
        إذا (avertexp->isDelete()) {
            VL_DO_DANGLING(avertexp->unlinkDelete(&m_break_گراف), avertexp);
        }
    }
}

فراغ گراف_Acyc::simplifyNone(گراف_Acyc_راس* avertexp) {
    // Don't need any vertices with no inputs, There's no way they can have a loop.
    // Likewise, vertices with no outputs
    إذا (avertexp->isDelete()) أرجع;
    إذا (avertexp->اندريون_خالي_جايون() || avertexp->ٻاهريون_خالي_جايون()) {
        UINFO(9, "  SimplifyNoneRemove " << avertexp);
        avertexp->سيٽ_Delete();  // Mark so we won't delete it twice
        // Remove edges
        طالما (V3_گراف_ڪنارو* ثابت edgep = avertexp->ٻاهريون_ڪنارا().frontp()) {
            work_ڌڪو(edgep->چوٽي());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        طالما (V3_گراف_ڪنارو* ثابت edgep = avertexp->اندريون_ڪنارا().frontp()) {
            work_ڌڪو(edgep->fromp());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
}

فراغ گراف_Acyc::simplifyOne(گراف_Acyc_راس* avertexp) {
    // If a node has one input and one output, we can remove it and change the edges
    إذا (avertexp->isDelete()) أرجع;
    إذا (avertexp->اندريون_Size1() && avertexp->ٻاهريون_Size1()) {
        V3_گراف_ڪنارو* ثابت اندريون_Edgep = avertexp->اندريون_ڪنارا().frontp();
        V3_گراف_ڪنارو* ثابت ٻاهريون_Edgep = avertexp->ٻاهريون_ڪنارا().frontp();
        V3_گراف_راس* ثابت اندريون_Vertexp = اندريون_Edgep->fromp();
        V3_گراف_راس* ثابت ٻاهريون_Vertexp = ٻاهريون_Edgep->چوٽي();
        // The in and out may be the same node; we'll make a loop
        // The in OR out may be THIS node; we can't delete it then.
        إذا (اندريون_Vertexp != avertexp && ٻاهريون_Vertexp != avertexp) {
            UINFO(9, "  SimplifyOneRemove " << avertexp);
            avertexp->سيٽ_Delete();  // Mark so we won't delete it twice
            // Make a new edge connecting the two vertices directly
            // If both are breakable, we pick the one with less weight, else it's arbitrary
            // We can forget about the origEdge list for the "non-selected" set of edges,
            // as we need to break only one set or the other set of edges, not both.
            // (This is why we must give preference to the cutable set.)
            V3_گراف_ڪنارو* ثابت templateEdgep
                = ((اندريون_Edgep->cutable()
                    && (!ٻاهريون_Edgep->cutable() || اندريون_Edgep->وزن_2() < ٻاهريون_Edgep->وزن_2()))
                       ? اندريون_Edgep
                       : ٻاهريون_Edgep);
            // cppcheck-suppress leakReturnValNotUsed
            ڪنارو_From_ڪنارو(templateEdgep, اندريون_Vertexp, ٻاهريون_Vertexp);
            // Remove old edge
            VL_DO_DANGLING(اندريون_Edgep->unlinkDelete(), اندريون_Edgep);
            VL_DO_DANGLING(ٻاهريون_Edgep->unlinkDelete(), ٻاهريون_Edgep);
            VL_DANGLING(templateEdgep);
            work_ڌڪو(اندريون_Vertexp);
            work_ڌڪو(ٻاهريون_Vertexp);
        }
    }
}

فراغ گراف_Acyc::simplify_ٻاهريون(گراف_Acyc_راس* avertexp) {
    // If a node has one output that's not cutable, all its inputs can be reassigned
    // to the next node in the list
    إذا (avertexp->isDelete()) أرجع;
    إذا (avertexp->ٻاهريون_Size1()) {
        V3_گراف_ڪنارو* ثابت ٻاهريون_Edgep = avertexp->ٻاهريون_ڪنارا().frontp();
        إذا (!ٻاهريون_Edgep->cutable()) {
            V3_گراف_راس* ٻاهريون_Vertexp = ٻاهريون_Edgep->چوٽي();
            UINFO(9, "  SimplifyOutRemove " << avertexp);
            avertexp->سيٽ_Delete();  // Mark so we won't delete it twice
            لكل (V3_گراف_ڪنارو* ثابت اندريون_Edgep : avertexp->اندريون_ڪنارا().unlinkable()) {
                V3_گراف_راس* اندريون_Vertexp = اندريون_Edgep->fromp();
                إذا (اندريون_Vertexp == avertexp) {
                    إذا (debug()) v3error("Non-cutable vertex=" << avertexp);  // LCOV_EXCL_LINE
                    v3error("Circular logic when ordering code (non-cutable edge loop)\n"
                            << m_origGraphp->reportLoops(  // calls OrderGraph::loopsVertexCb
                                   &V3_گراف_ڪنارو::followNotCutable, avertexp->origVertexp()));
                    // Things are unlikely to end well at this point,
                    // but we'll try something to get to further errors...
                    اندريون_Edgep->cutable(سچ);
                    أرجع;
                }
                // Make a new edge connecting the two vertices directly
                // cppcheck-suppress leakReturnValNotUsed
                ڪنارو_From_ڪنارو(اندريون_Edgep, اندريون_Vertexp, ٻاهريون_Vertexp);
                // Remove old edge
                VL_DO_DANGLING(اندريون_Edgep->unlinkDelete(), اندريون_Edgep);
                work_ڌڪو(اندريون_Vertexp);
            }
            VL_DO_DANGLING(ٻاهريون_Edgep->unlinkDelete(), ٻاهريون_Edgep);
            work_ڌڪو(ٻاهريون_Vertexp);
        }
    }
}

فراغ گراف_Acyc::simplifyDup(گراف_Acyc_راس* avertexp) {
    // Remove redundant edges
    إذا (avertexp->isDelete()) أرجع;
    // Clear marks
    لكل (V3_گراف_ڪنارو& ڪنارو : avertexp->ٻاهريون_ڪنارا()) ڪنارو.چوٽي()->userp(خالي_اشارو);
    // Mark edges and detect duplications
    لكل (V3_گراف_ڪنارو* ثابت edgep : avertexp->ٻاهريون_ڪنارا().unlinkable()) {
        V3_گراف_راس* ٻاهريون_Vertexp = edgep->چوٽي();
        V3_گراف_ڪنارو* prevEdgep = static_cast<V3_گراف_ڪنارو*>(ٻاهريون_Vertexp->userp());
        إذا (prevEdgep) {
            إذا (!prevEdgep->cutable()) {
                // !cutable duplicates prev !cutable: we can ignore it, redundant
                //  cutable duplicates prev !cutable: know it's not a relevant loop, ignore it
                UINFO(8, "    DelDupEdge " << avertexp << " -> " << edgep->چوٽي());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            } وإلا إذا (!edgep->cutable()) {
                // !cutable duplicates prev  cutable: delete the earlier cutable
                UINFO(8, "    DelDupPrev " << avertexp << " -> " << prevEdgep->چوٽي());
                VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                ٻاهريون_Vertexp->userp(edgep);
            } وإلا {
                //  cutable duplicates prev  cutable: combine weights
                UINFO(8, "    DelDupComb " << avertexp << " -> " << edgep->چوٽي());
                prevEdgep->وزن_2(prevEdgep->وزن_2() + edgep->وزن_2());
                جوڙيو_Orig_Edgep(prevEdgep, edgep);
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            work_ڌڪو(ٻاهريون_Vertexp);
            work_ڌڪو(avertexp);
        } وإلا {
            // No previous assignment
            ٻاهريون_Vertexp->userp(edgep);
        }
    }
}

فراغ گراف_Acyc::cutBasic(گراف_Acyc_راس* avertexp) {
    // Detect and cleanup any loops from node to itself
    إذا (avertexp->isDelete()) أرجع;
    لكل (V3_گراف_ڪنارو* ثابت edgep : avertexp->ٻاهريون_ڪنارا().unlinkable()) {
        إذا (edgep->cutable() && edgep->چوٽي() == avertexp) {
            cut_Orig_ڪنارو(edgep, "  Cut Basic");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_ڌڪو(avertexp);
        }
    }
}

فراغ گراف_Acyc::cutBackward(گراف_Acyc_راس* avertexp) {
    // If a cutable edge is from A->B, and there's a non-cutable edge B->A, then must cut!
    إذا (avertexp->isDelete()) أرجع;
    // Clear marks
    لكل (V3_گراف_ڪنارو& ڪنارو : avertexp->ٻاهريون_ڪنارا()) ڪنارو.چوٽي()->user(خطأ);
    لكل (V3_گراف_ڪنارو& ڪنارو : avertexp->اندريون_ڪنارا()) {
        إذا (!ڪنارو.cutable()) ڪنارو.fromp()->user(سچ);
    }
    // Detect duplications
    لكل (V3_گراف_ڪنارو* ثابت edgep : avertexp->ٻاهريون_ڪنارا().unlinkable()) {
        إذا (edgep->cutable() && edgep->چوٽي()->user()) {
            cut_Orig_ڪنارو(edgep, "  Cut A->B->A");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_ڌڪو(avertexp);
        }
    }
}

فراغ گراف_Acyc::رکو_2() {
    // Input is m_breakGraph with ranks already assigned on non-breakable edges

    // Make a list of all cutable edges in the graph
    صحيح انگ_ڪنارا = 0;
    لكل (V3_گراف_راس& راس : m_break_گراف.vertices()) {
        لكل (V3_گراف_ڪنارو& ڪنارو : راس.ٻاهريون_ڪنارا()) {
            إذا (ڪنارو.وزن_2() && ڪنارو.cutable()) ++انگ_ڪنارا;
        }
    }
    UINFO(4, "    Cutable edges = " << انگ_ڪنارا);

    std::ويڪٽر<V3_گراف_ڪنارو*> ڪنارا;  // List of all edges to be processed
    // Make the vector properly sized right off the bat -- faster than reallocating
    ڪنارا.reserve(انگ_ڪنارا + 1);
    لكل (V3_گراف_راس& راس : m_break_گراف.vertices()) {
        راس.user(0);  // Clear in prep of next step
        لكل (V3_گراف_ڪنارو& ڪنارو : راس.ٻاهريون_ڪنارا()) {
            إذا (ڪنارو.وزن_2() && ڪنارو.cutable()) ڪنارا.ڌڪو_پٺ(&ڪنارو);
        }
    }

    // Sort by weight, then by vertex (so that we completely process one vertex, when possible)
    stable_ترتيب_ڏيو(ڪنارا.begin(), ڪنارا.پڇاڙي(), گراف_Acyc_ڪنارو_Cmp());

    // Process each edge in weighted order
    m_رکو_قدم = 10;
    لكل (V3_گراف_ڪنارو* edgep : ڪنارا) رکو_Try_ڪنارو(edgep);
}

فراغ گراف_Acyc::رکو_Try_ڪنارو(V3_گراف_ڪنارو* edgep) {
    // Try to make this edge uncutable
    m_رکو_قدم++;
    UINFO(8, "    PlaceEdge s" << m_رکو_قدم << " w" << edgep->وزن_2() << " " << edgep->fromp());
    // Make the edge uncutable so we detect it in placement
    edgep->cutable(خطأ);
    // Vertex::m_user begin: number indicates this edge was completed
    // Try to assign ranks, presuming this edge is in place
    // If we come across user()==placestep, we've detected a loop and must back out
    ثابت منطقي loop
        = رکو_Iterate(static_cast<گراف_Acyc_راس*>(edgep->چوٽي()), edgep->fromp()->rank() + 1);
    إذا (!loop) {
        // No loop, we can keep it as uncutable
        // Commit the new ranks we calculated
        // Just cleanup the list.  If this is slow, we can add another set of
        // user counters to avoid cleaning up the list.
        طالما (workBeginp()) work_ڪڍو();
    } وإلا {
        // Adding this edge would cause a loop, kill it
        edgep->cutable(سچ);  // So graph still looks pretty
        cut_Orig_ڪنارو(edgep, "  Cut loop");
        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        // Back out the ranks we calculated
        طالما (گراف_Acyc_راس* vertexp = workBeginp()) {
            work_ڪڍو();
            vertexp->rank(vertexp->m_storedRank);
        }
    }
}

منطقي گراف_Acyc::رکو_Iterate(گراف_Acyc_راس* vertexp, uint32_t currentRank) {
    // Assign rank to each unvisited node
    //   rank() is the "committed rank" of the graph known without loops
    // If larger rank is found, assign it and loop back through
    // If we hit a back node make a list of all loops
    إذا (vertexp->rank() >= currentRank) أرجع خطأ;  // Already processed it
    إذا (vertexp->user() == m_رکو_قدم) أرجع سچ;  // Loop detected
    vertexp->user(m_رکو_قدم);
    // Remember we're changing the rank of this node; might need to back out
    إذا (!vertexp->m_onWorkList) {
        vertexp->m_storedRank = vertexp->rank();
        work_ڌڪو(vertexp);
    }
    vertexp->rank(currentRank);
    // Follow all edges and increase their ranks
    لكل (V3_گراف_ڪنارو& ڪنارو : vertexp->ٻاهريون_ڪنارا()) {
        إذا (ڪنارو.وزن_2() && !ڪنارو.cutable()) {
            إذا (رکو_Iterate(static_cast<گراف_Acyc_راس*>(ڪنارو.چوٽي()), currentRank + 1)) {
                // We don't need to reset user(); we'll use a different placeStep for the next edge
                أرجع سچ;  // Loop detected
            }
        }
    }
    vertexp->user(0);
    أرجع خطأ;
}

//----- Main algorithm entry point

فراغ گراف_Acyc::مکيه() {
    m_break_گراف.user_Clear_ڪنارا();

    // Color based on possible loops
    m_origGraphp->stronglyConnected(m_orig_ڪنارو_Funcp);

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
    simplify(سچ);
    إذا (dump_گراف_Level() >= 6) m_break_گراف.dumpDotFilePrefixed("acyc_mid");

    UINFO(4, " Ranking");
    m_break_گراف.rank(&V3_گراف_ڪنارو::followNotCutable);
    إذا (dump_گراف_Level() >= 6) m_break_گراف.dumpDotFilePrefixed("acyc_rank");

    UINFO(4, " Placement");
    رکو_2();
    إذا (dump_گراف_Level() >= 6) m_break_گراف.dumpDotFilePrefixed("acyc_place");

    UINFO(4, " Final Ranking");
    // Only needed to assert there are no loops in completed graph
    m_break_گراف.rank(&V3_گراف_ڪنارو::followAlwaysTrue);
    إذا (dump_گراف_Level() >= 6) m_break_گراف.dumpDotFilePrefixed("acyc_done");
}

فراغ V3_گراف::acyclic(V3_ڪنارو_Func_P ڪنارو_Funcp) {
    UINFO(4, "Acyclic");
    گراف_Acyc acyc{هذا, ڪنارو_Funcp};
    acyc.مکيه();
    UINFO(4, "Acyclic done");
}
