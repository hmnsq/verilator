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

#تعریف_کرو VL_MT_DISABLED_کوڈ_UNIT 1

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
    یار صنف گراف_Acyc;
    V3ListLinks<گراف_Acyc_راس> m_links;  // List links to store instances of this class
    uint32_t m_storedRank = 0;  // Rank held until commit to edge placement
    منطقي m_onWorkList = خطأ;  // True if already on list of work to do
    منطقي m_deleted = خطأ;  // True if deleted

خاص:
    V3ListLinks<گراف_Acyc_راس>& links() { أرجع m_links; }

عام:
    // List type to store instances of this class
    ورتوں List = V3List<گراف_Acyc_راس, &گراف_Acyc_راس::links>;

    گراف_Acyc_راس(V3_گراف* graphp, V3_گراف_راس* origVertexp)
        : V3_گراف_راس{graphp}
        , m_origVertexp{origVertexp} {}
    ~گراف_Acyc_راس() override = افتراضي;
    V3_گراف_راس* origVertexp() ثابت { أرجع m_origVertexp; }
    فراغ سیٹ_Delete() { m_deleted = سچ; }
    منطقي isDelete() ثابت { أرجع m_deleted; }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string name() ثابت override { أرجع m_origVertexp->name(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string dotColor() ثابت override { أرجع m_origVertexp->dotColor(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    File_سطر* fileline() ثابت override { أرجع m_origVertexp->fileline(); }
};

//--------------------------------------------------------------------

صنف گراف_Acyc_کنارا final : عام V3_گراف_کنارا {
    VL_RTTI_IMPL(گراف_Acyc_کنارا, V3_گراف_کنارا)
    // userp() is always used to point to the head original graph edge
خاص:
    ورتوں Orig_کنارا_List = std::list<V3_گراف_کنارا*>;  // List of orig edges, see also GraphAcyc's decl
    V3_گراف_کنارا* origEdgep() ثابت {
        ثابت Orig_کنارا_List* ثابت oEListp = static_cast<Orig_کنارا_List*>(userp());
        UASSERT(oEListp, "No original edge associated with acyc edge " << هذا);
        أرجع (oEListp->front());
    }

عام:
    گراف_Acyc_کنارا(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی, صحيح بھار_2,
                  منطقي cutable = خطأ)
        : V3_گراف_کنارا{graphp, fromp, چوٹی, بھار_2, cutable} {}
    ~گراف_Acyc_کنارا() override = افتراضي;
    // yellow=we might still cut it, else oldEdge: yellowGreen=made uncutable, red=uncutable
    string dotColor() ثابت override { أرجع (cutable() ? "yellow" : origEdgep()->dotColor()); }
};

//--------------------------------------------------------------------

بنية گراف_Acyc_کنارا_Cmp final {
    منطقي عامل()(ثابت V3_گراف_کنارا* lhsp, ثابت V3_گراف_کنارا* rhsp) ثابت {
        إذا (lhsp->بھار_2() > rhsp->بھار_2()) أرجع سچ;  // LHS goes first
        إذا (lhsp->بھار_2() < rhsp->بھار_2()) أرجع خطأ;  // RHS goes first
        أرجع خطأ;
    }
};

//--------------------------------------------------------------------

// CLASSES
صنف گراف_Acyc final {
    ورتوں Orig_کنارا_List
        = std::list<V3_گراف_کنارا*>;  // List of orig edges, see also GraphAcycEdge's decl
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
    std::سمتیہ<Orig_کنارا_List*> m_orig_کنارا_Delp;  // List of deletions to do when done
    ثابت V3_کنارا_Func_P
        m_orig_کنارا_Funcp;  // Function that says we follow this edge (in original graph)
    uint32_t m_رکھو_قدم = 0;  // Number that user() must be equal to to indicate processing

    // METHODS
    فراغ build_گراف(V3_گراف* origGraphp);
    فراغ build_گراف_Iterate(V3_گراف_راس* overtexp, گراف_Acyc_راس* avertexp);
    فراغ simplify(منطقي allowCut);
    فراغ simplifyNone(گراف_Acyc_راس* avertexp);
    فراغ simplifyOne(گراف_Acyc_راس* avertexp);
    فراغ simplify_باہرلا(گراف_Acyc_راس* avertexp);
    فراغ simplifyDup(گراف_Acyc_راس* avertexp);
    فراغ cutBasic(گراف_Acyc_راس* avertexp);
    فراغ cutBackward(گراف_Acyc_راس* avertexp);
    فراغ deleteMarked();
    فراغ رکھو_2();
    فراغ رکھو_Try_کنارا(V3_گراف_کنارا* edgep);
    منطقي رکھو_Iterate(گراف_Acyc_راس* vertexp, uint32_t currentRank);

    منطقي orig_Follow_کنارا(V3_گراف_کنارا* edgep) {
        أرجع (edgep->بھار_2() && (m_orig_کنارا_Funcp)(edgep));
    }
    فراغ کنارا_From_کنارا(V3_گراف_کنارا* oldedgep, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی) {
        // Make new breakGraph edge, with old edge as a template
        گراف_Acyc_کنارا* ثابت newEdgep = جديد گراف_Acyc_کنارا{&m_break_گراف, fromp, چوٹی,
                                                          oldedgep->بھار_2(), oldedgep->cutable()};
        newEdgep->userp(oldedgep->userp());  // Keep pointer to OrigEdgeList
    }
    فراغ جوڑو_Orig_Edgep(V3_گراف_کنارا* منزل_Edgep, V3_گراف_کنارا* جوڑو_Edgep) {
        // Add addEdge (or it's list) to list of edges that break edge represents
        // Note addEdge may already have a bunch of similar linked edge representations.  Yuk.
        UASSERT(جوڑو_Edgep, "Adding nullptr");
        إذا (!منزل_Edgep->userp()) {
            Orig_کنارا_List* ثابت oep = جديد Orig_کنارا_List;
            m_orig_کنارا_Delp.ڈالو_پٹھ(oep);
            منزل_Edgep->userp(oep);
        }
        Orig_کنارا_List* ثابت oEListp = static_cast<Orig_کنارا_List*>(منزل_Edgep->userp());
        إذا (Orig_کنارا_List* ثابت جوڑو_Listp = static_cast<Orig_کنارا_List*>(جوڑو_Edgep->userp())) {
            لكل (ثابت خودکار& itr : *جوڑو_Listp) oEListp->ڈالو_پٹھ(itr);
            جوڑو_Listp->clear();  // Done with it
        } وإلا {
            oEListp->ڈالو_پٹھ(جوڑو_Edgep);
        }
    }
    فراغ cut_Orig_کنارا(V3_گراف_کنارا* breakEdgep, ثابت حرف* why) {
        // From the break edge, cut edges in original graph it represents
        UINFO(8, why << " CUT " << breakEdgep->fromp());
        breakEdgep->cut();
        ثابت Orig_کنارا_List* ثابت oEListp = static_cast<Orig_کنارا_List*>(breakEdgep->userp());
        إذا (!oEListp) {
            v3fatalSrc("No original edge associated with cutting edge " << breakEdgep);
        }
        // The breakGraph edge may represent multiple real edges; cut them all
        لكل (ثابت خودکار& origEdgep : *oEListp) {
            origEdgep->cut();
            UINFO(8, "  " << why << "   " << origEdgep->fromp() << " ->" << origEdgep->چوٹی());
        }
    }
    // Work Queue
    فراغ work_ڈالو(V3_گراف_راس* vertexp) {
        گراف_Acyc_راس* ثابت avertexp = static_cast<گراف_Acyc_راس*>(vertexp);
        // Add vertex to list of nodes needing further optimization trials
        إذا (!avertexp->m_onWorkList) {
            avertexp->m_onWorkList = سچ;
            m_work.link_پٹھ(avertexp);
        }
    }
    گراف_Acyc_راس* workBeginp() { أرجع m_work.frontp(); }
    فراغ work_نکالو() {
        گراف_Acyc_راس* ثابت avertexp = workBeginp();
        avertexp->m_onWorkList = خطأ;
        m_work.unlink(avertexp);
    }

عام:
    // CONSTRUCTORS
    گراف_Acyc(V3_گراف* origGraphp, V3_کنارا_Func_P کنارا_Funcp)
        : m_origGraphp{origGraphp}
        , m_orig_کنارا_Funcp{کنارا_Funcp} {}
    ~گراف_Acyc() {
        لكل (Orig_کنارا_List* ip : m_orig_کنارا_Delp) مٹاؤ ip;
        m_orig_کنارا_Delp.clear();
    }
    فراغ مکھ();
};

//--------------------------------------------------------------------

فراغ گراف_Acyc::build_گراف(V3_گراف* origGraphp) {
    // Presumes the graph has been strongly ordered,
    // and thus there's a unique color if there are loops in this subgraph.

    // For each old node, make a new graph node for optimization
    origGraphp->userClearVertices();
    origGraphp->user_Clear_کنارے();
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
    لكل (V3_گراف_کنارا& کنارا : overtexp->باہرلا_کنارے()) {
        إذا (orig_Follow_کنارا(&کنارا)) {  // not cut
            ثابت V3_گراف_راس* منزل_Vertexp = کنارا.چوٹی();
            إذا (منزل_Vertexp->color()) {
                گراف_Acyc_راس* ثابت منزل_AVertexp
                    = static_cast<گراف_Acyc_راس*>(منزل_Vertexp->userp());
                // Replicate the old edge into the new graph
                // There may be multiple edges between same pairs of vertices
                V3_گراف_کنارا* breakEdgep = جديد گراف_Acyc_کنارا{&m_break_گراف, avertexp, منزل_AVertexp,
                                                            کنارا.بھار_2(), کنارا.cutable()};
                جوڑو_Orig_Edgep(breakEdgep, &کنارا);  // So can find original edge
            }
        }
    }
}

فراغ گراف_Acyc::simplify(منطقي allowCut) {
    // Add all nodes to list of work to do
    لكل (V3_گراف_راس& راس : m_break_گراف.vertices()) work_ڈالو(&راس);
    // Optimize till everything finished
    طالما (گراف_Acyc_راس* vertexp = workBeginp()) {
        work_نکالو();
        simplifyNone(vertexp);
        simplifyOne(vertexp);
        simplify_باہرلا(vertexp);
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
    إذا (avertexp->وچ_خالی_تھاواں() || avertexp->باہرلا_خالی_تھاواں()) {
        UINFO(9, "  SimplifyNoneRemove " << avertexp);
        avertexp->سیٹ_Delete();  // Mark so we won't delete it twice
        // Remove edges
        طالما (V3_گراف_کنارا* ثابت edgep = avertexp->باہرلا_کنارے().frontp()) {
            work_ڈالو(edgep->چوٹی());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        طالما (V3_گراف_کنارا* ثابت edgep = avertexp->وچ_کنارے().frontp()) {
            work_ڈالو(edgep->fromp());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
}

فراغ گراف_Acyc::simplifyOne(گراف_Acyc_راس* avertexp) {
    // If a node has one input and one output, we can remove it and change the edges
    إذا (avertexp->isDelete()) أرجع;
    إذا (avertexp->وچ_Size1() && avertexp->باہرلا_Size1()) {
        V3_گراف_کنارا* ثابت وچ_Edgep = avertexp->وچ_کنارے().frontp();
        V3_گراف_کنارا* ثابت باہرلا_Edgep = avertexp->باہرلا_کنارے().frontp();
        V3_گراف_راس* ثابت وچ_Vertexp = وچ_Edgep->fromp();
        V3_گراف_راس* ثابت باہرلا_Vertexp = باہرلا_Edgep->چوٹی();
        // The in and out may be the same node; we'll make a loop
        // The in OR out may be THIS node; we can't delete it then.
        إذا (وچ_Vertexp != avertexp && باہرلا_Vertexp != avertexp) {
            UINFO(9, "  SimplifyOneRemove " << avertexp);
            avertexp->سیٹ_Delete();  // Mark so we won't delete it twice
            // Make a new edge connecting the two vertices directly
            // If both are breakable, we pick the one with less weight, else it's arbitrary
            // We can forget about the origEdge list for the "non-selected" set of edges,
            // as we need to break only one set or the other set of edges, not both.
            // (This is why we must give preference to the cutable set.)
            V3_گراف_کنارا* ثابت templateEdgep
                = ((وچ_Edgep->cutable()
                    && (!باہرلا_Edgep->cutable() || وچ_Edgep->بھار_2() < باہرلا_Edgep->بھار_2()))
                       ? وچ_Edgep
                       : باہرلا_Edgep);
            // cppcheck-suppress leakReturnValNotUsed
            کنارا_From_کنارا(templateEdgep, وچ_Vertexp, باہرلا_Vertexp);
            // Remove old edge
            VL_DO_DANGLING(وچ_Edgep->unlinkDelete(), وچ_Edgep);
            VL_DO_DANGLING(باہرلا_Edgep->unlinkDelete(), باہرلا_Edgep);
            VL_DANGLING(templateEdgep);
            work_ڈالو(وچ_Vertexp);
            work_ڈالو(باہرلا_Vertexp);
        }
    }
}

فراغ گراف_Acyc::simplify_باہرلا(گراف_Acyc_راس* avertexp) {
    // If a node has one output that's not cutable, all its inputs can be reassigned
    // to the next node in the list
    إذا (avertexp->isDelete()) أرجع;
    إذا (avertexp->باہرلا_Size1()) {
        V3_گراف_کنارا* ثابت باہرلا_Edgep = avertexp->باہرلا_کنارے().frontp();
        إذا (!باہرلا_Edgep->cutable()) {
            V3_گراف_راس* باہرلا_Vertexp = باہرلا_Edgep->چوٹی();
            UINFO(9, "  SimplifyOutRemove " << avertexp);
            avertexp->سیٹ_Delete();  // Mark so we won't delete it twice
            لكل (V3_گراف_کنارا* ثابت وچ_Edgep : avertexp->وچ_کنارے().unlinkable()) {
                V3_گراف_راس* وچ_Vertexp = وچ_Edgep->fromp();
                إذا (وچ_Vertexp == avertexp) {
                    إذا (debug()) v3error("Non-cutable vertex=" << avertexp);  // LCOV_EXCL_LINE
                    v3error("Circular logic when ordering code (non-cutable edge loop)\n"
                            << m_origGraphp->reportLoops(  // calls OrderGraph::loopsVertexCb
                                   &V3_گراف_کنارا::followNotCutable, avertexp->origVertexp()));
                    // Things are unlikely to end well at this point,
                    // but we'll try something to get to further errors...
                    وچ_Edgep->cutable(سچ);
                    أرجع;
                }
                // Make a new edge connecting the two vertices directly
                // cppcheck-suppress leakReturnValNotUsed
                کنارا_From_کنارا(وچ_Edgep, وچ_Vertexp, باہرلا_Vertexp);
                // Remove old edge
                VL_DO_DANGLING(وچ_Edgep->unlinkDelete(), وچ_Edgep);
                work_ڈالو(وچ_Vertexp);
            }
            VL_DO_DANGLING(باہرلا_Edgep->unlinkDelete(), باہرلا_Edgep);
            work_ڈالو(باہرلا_Vertexp);
        }
    }
}

فراغ گراف_Acyc::simplifyDup(گراف_Acyc_راس* avertexp) {
    // Remove redundant edges
    إذا (avertexp->isDelete()) أرجع;
    // Clear marks
    لكل (V3_گراف_کنارا& کنارا : avertexp->باہرلا_کنارے()) کنارا.چوٹی()->userp(خالی_سوچک);
    // Mark edges and detect duplications
    لكل (V3_گراف_کنارا* ثابت edgep : avertexp->باہرلا_کنارے().unlinkable()) {
        V3_گراف_راس* باہرلا_Vertexp = edgep->چوٹی();
        V3_گراف_کنارا* prevEdgep = static_cast<V3_گراف_کنارا*>(باہرلا_Vertexp->userp());
        إذا (prevEdgep) {
            إذا (!prevEdgep->cutable()) {
                // !cutable duplicates prev !cutable: we can ignore it, redundant
                //  cutable duplicates prev !cutable: know it's not a relevant loop, ignore it
                UINFO(8, "    DelDupEdge " << avertexp << " -> " << edgep->چوٹی());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            } وإلا إذا (!edgep->cutable()) {
                // !cutable duplicates prev  cutable: delete the earlier cutable
                UINFO(8, "    DelDupPrev " << avertexp << " -> " << prevEdgep->چوٹی());
                VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                باہرلا_Vertexp->userp(edgep);
            } وإلا {
                //  cutable duplicates prev  cutable: combine weights
                UINFO(8, "    DelDupComb " << avertexp << " -> " << edgep->چوٹی());
                prevEdgep->بھار_2(prevEdgep->بھار_2() + edgep->بھار_2());
                جوڑو_Orig_Edgep(prevEdgep, edgep);
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            work_ڈالو(باہرلا_Vertexp);
            work_ڈالو(avertexp);
        } وإلا {
            // No previous assignment
            باہرلا_Vertexp->userp(edgep);
        }
    }
}

فراغ گراف_Acyc::cutBasic(گراف_Acyc_راس* avertexp) {
    // Detect and cleanup any loops from node to itself
    إذا (avertexp->isDelete()) أرجع;
    لكل (V3_گراف_کنارا* ثابت edgep : avertexp->باہرلا_کنارے().unlinkable()) {
        إذا (edgep->cutable() && edgep->چوٹی() == avertexp) {
            cut_Orig_کنارا(edgep, "  Cut Basic");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_ڈالو(avertexp);
        }
    }
}

فراغ گراف_Acyc::cutBackward(گراف_Acyc_راس* avertexp) {
    // If a cutable edge is from A->B, and there's a non-cutable edge B->A, then must cut!
    إذا (avertexp->isDelete()) أرجع;
    // Clear marks
    لكل (V3_گراف_کنارا& کنارا : avertexp->باہرلا_کنارے()) کنارا.چوٹی()->user(خطأ);
    لكل (V3_گراف_کنارا& کنارا : avertexp->وچ_کنارے()) {
        إذا (!کنارا.cutable()) کنارا.fromp()->user(سچ);
    }
    // Detect duplications
    لكل (V3_گراف_کنارا* ثابت edgep : avertexp->باہرلا_کنارے().unlinkable()) {
        إذا (edgep->cutable() && edgep->چوٹی()->user()) {
            cut_Orig_کنارا(edgep, "  Cut A->B->A");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_ڈالو(avertexp);
        }
    }
}

فراغ گراف_Acyc::رکھو_2() {
    // Input is m_breakGraph with ranks already assigned on non-breakable edges

    // Make a list of all cutable edges in the graph
    صحيح عدد_کنارے = 0;
    لكل (V3_گراف_راس& راس : m_break_گراف.vertices()) {
        لكل (V3_گراف_کنارا& کنارا : راس.باہرلا_کنارے()) {
            إذا (کنارا.بھار_2() && کنارا.cutable()) ++عدد_کنارے;
        }
    }
    UINFO(4, "    Cutable edges = " << عدد_کنارے);

    std::سمتیہ<V3_گراف_کنارا*> کنارے;  // List of all edges to be processed
    // Make the vector properly sized right off the bat -- faster than reallocating
    کنارے.reserve(عدد_کنارے + 1);
    لكل (V3_گراف_راس& راس : m_break_گراف.vertices()) {
        راس.user(0);  // Clear in prep of next step
        لكل (V3_گراف_کنارا& کنارا : راس.باہرلا_کنارے()) {
            إذا (کنارا.بھار_2() && کنارا.cutable()) کنارے.ڈالو_پٹھ(&کنارا);
        }
    }

    // Sort by weight, then by vertex (so that we completely process one vertex, when possible)
    stable_ترتیب_دیو(کنارے.begin(), کنارے.سرا(), گراف_Acyc_کنارا_Cmp());

    // Process each edge in weighted order
    m_رکھو_قدم = 10;
    لكل (V3_گراف_کنارا* edgep : کنارے) رکھو_Try_کنارا(edgep);
}

فراغ گراف_Acyc::رکھو_Try_کنارا(V3_گراف_کنارا* edgep) {
    // Try to make this edge uncutable
    m_رکھو_قدم++;
    UINFO(8, "    PlaceEdge s" << m_رکھو_قدم << " w" << edgep->بھار_2() << " " << edgep->fromp());
    // Make the edge uncutable so we detect it in placement
    edgep->cutable(خطأ);
    // Vertex::m_user begin: number indicates this edge was completed
    // Try to assign ranks, presuming this edge is in place
    // If we come across user()==placestep, we've detected a loop and must back out
    ثابت منطقي loop
        = رکھو_Iterate(static_cast<گراف_Acyc_راس*>(edgep->چوٹی()), edgep->fromp()->rank() + 1);
    إذا (!loop) {
        // No loop, we can keep it as uncutable
        // Commit the new ranks we calculated
        // Just cleanup the list.  If this is slow, we can add another set of
        // user counters to avoid cleaning up the list.
        طالما (workBeginp()) work_نکالو();
    } وإلا {
        // Adding this edge would cause a loop, kill it
        edgep->cutable(سچ);  // So graph still looks pretty
        cut_Orig_کنارا(edgep, "  Cut loop");
        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        // Back out the ranks we calculated
        طالما (گراف_Acyc_راس* vertexp = workBeginp()) {
            work_نکالو();
            vertexp->rank(vertexp->m_storedRank);
        }
    }
}

منطقي گراف_Acyc::رکھو_Iterate(گراف_Acyc_راس* vertexp, uint32_t currentRank) {
    // Assign rank to each unvisited node
    //   rank() is the "committed rank" of the graph known without loops
    // If larger rank is found, assign it and loop back through
    // If we hit a back node make a list of all loops
    إذا (vertexp->rank() >= currentRank) أرجع خطأ;  // Already processed it
    إذا (vertexp->user() == m_رکھو_قدم) أرجع سچ;  // Loop detected
    vertexp->user(m_رکھو_قدم);
    // Remember we're changing the rank of this node; might need to back out
    إذا (!vertexp->m_onWorkList) {
        vertexp->m_storedRank = vertexp->rank();
        work_ڈالو(vertexp);
    }
    vertexp->rank(currentRank);
    // Follow all edges and increase their ranks
    لكل (V3_گراف_کنارا& کنارا : vertexp->باہرلا_کنارے()) {
        إذا (کنارا.بھار_2() && !کنارا.cutable()) {
            إذا (رکھو_Iterate(static_cast<گراف_Acyc_راس*>(کنارا.چوٹی()), currentRank + 1)) {
                // We don't need to reset user(); we'll use a different placeStep for the next edge
                أرجع سچ;  // Loop detected
            }
        }
    }
    vertexp->user(0);
    أرجع خطأ;
}

//----- Main algorithm entry point

فراغ گراف_Acyc::مکھ() {
    m_break_گراف.user_Clear_کنارے();

    // Color based on possible loops
    m_origGraphp->stronglyConnected(m_orig_کنارا_Funcp);

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
    m_break_گراف.rank(&V3_گراف_کنارا::followNotCutable);
    إذا (dump_گراف_Level() >= 6) m_break_گراف.dumpDotFilePrefixed("acyc_rank");

    UINFO(4, " Placement");
    رکھو_2();
    إذا (dump_گراف_Level() >= 6) m_break_گراف.dumpDotFilePrefixed("acyc_place");

    UINFO(4, " Final Ranking");
    // Only needed to assert there are no loops in completed graph
    m_break_گراف.rank(&V3_گراف_کنارا::followAlwaysTrue);
    إذا (dump_گراف_Level() >= 6) m_break_گراف.dumpDotFilePrefixed("acyc_done");
}

فراغ V3_گراف::acyclic(V3_کنارا_Func_P کنارا_Funcp) {
    UINFO(4, "Acyclic");
    گراف_Acyc acyc{هذا, کنارا_Funcp};
    acyc.مکھ();
    UINFO(4, "Acyclic done");
}
