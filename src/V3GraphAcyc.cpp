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

#تعریف_کرو VL_MT_DISABLED_رمز_UNIT 1

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Global.h"
#شامل "V3Graph.h"

#شامل <algorithm>
#شامل <list>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - acyclic
//      Break the minimal number of backward edges to make the graph acyclic

جماعت گراف_Acyc_راس final : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(گراف_Acyc_راس, V3_گراف_راس)
    // user() is used for various sub-algorithm pieces
    V3_گراف_راس* ثابت m_origVertexp;  // Pointer to first vertex this represents
محفوظ:
    دوست جماعت گراف_Acyc;
    V3ListLinks<گراف_Acyc_راس> m_links;  // List links to store instances of this class
    uint32_t m_storedRank = 0;  // Rank held until commit to edge placement
    منطقی m_onWorkList = جھوٹ;  // True if already on list of work to do
    منطقی m_deleted = جھوٹ;  // True if deleted

نجی:
    V3ListLinks<گراف_Acyc_راس>& links() { واپس m_links; }

عوامی:
    // List type to store instances of this class
    استعمال List = V3List<گراف_Acyc_راس, &گراف_Acyc_راس::links>;

    گراف_Acyc_راس(V3_گراف* graphp, V3_گراف_راس* origVertexp)
        : V3_گراف_راس{graphp}
        , m_origVertexp{origVertexp} {}
    ~گراف_Acyc_راس() override = طےشدہ;
    V3_گراف_راس* origVertexp() ثابت { واپس m_origVertexp; }
    خالی سیٹ_Delete() { m_deleted = سچ; }
    منطقی isDelete() ثابت { واپس m_deleted; }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string name() ثابت override { واپس m_origVertexp->name(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string dotColor() ثابت override { واپس m_origVertexp->dotColor(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    File_سطر* fileline() ثابت override { واپس m_origVertexp->fileline(); }
};

//--------------------------------------------------------------------

جماعت گراف_Acyc_کنارہ final : عوامی V3_گراف_کنارہ {
    VL_RTTI_IMPL(گراف_Acyc_کنارہ, V3_گراف_کنارہ)
    // userp() is always used to point to the head original graph edge
نجی:
    استعمال Orig_کنارہ_List = std::list<V3_گراف_کنارہ*>;  // List of orig edges, see also GraphAcyc's decl
    V3_گراف_کنارہ* origEdgep() ثابت {
        ثابت Orig_کنارہ_List* ثابت oEListp = static_cast<Orig_کنارہ_List*>(userp());
        UASSERT(oEListp, "No original edge associated with acyc edge " << یہ);
        واپس (oEListp->front());
    }

عوامی:
    گراف_Acyc_کنارہ(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی, صحیح وزن_2,
                  منطقی cutable = جھوٹ)
        : V3_گراف_کنارہ{graphp, fromp, چوٹی, وزن_2, cutable} {}
    ~گراف_Acyc_کنارہ() override = طےشدہ;
    // yellow=we might still cut it, else oldEdge: yellowGreen=made uncutable, red=uncutable
    string dotColor() ثابت override { واپس (cutable() ? "yellow" : origEdgep()->dotColor()); }
};

//--------------------------------------------------------------------

ساخت گراف_Acyc_کنارہ_Cmp final {
    منطقی عامل()(ثابت V3_گراف_کنارہ* lhsp, ثابت V3_گراف_کنارہ* rhsp) ثابت {
        اگر (lhsp->وزن_2() > rhsp->وزن_2()) واپس سچ;  // LHS goes first
        اگر (lhsp->وزن_2() < rhsp->وزن_2()) واپس جھوٹ;  // RHS goes first
        واپس جھوٹ;
    }
};

//--------------------------------------------------------------------

// CLASSES
جماعت گراف_Acyc final {
    استعمال Orig_کنارہ_List
        = std::list<V3_گراف_کنارہ*>;  // List of orig edges, see also GraphAcycEdge's decl
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
    std::سمتیہ<Orig_کنارہ_List*> m_orig_کنارہ_Delp;  // List of deletions to do when done
    ثابت V3_کنارہ_Func_P
        m_orig_کنارہ_Funcp;  // Function that says we follow this edge (in original graph)
    uint32_t m_رکھو_قدم = 0;  // Number that user() must be equal to to indicate processing

    // METHODS
    خالی build_گراف(V3_گراف* origGraphp);
    خالی build_گراف_Iterate(V3_گراف_راس* overtexp, گراف_Acyc_راس* avertexp);
    خالی simplify(منطقی allowCut);
    خالی simplifyNone(گراف_Acyc_راس* avertexp);
    خالی simplifyOne(گراف_Acyc_راس* avertexp);
    خالی simplify_برآمد(گراف_Acyc_راس* avertexp);
    خالی simplifyDup(گراف_Acyc_راس* avertexp);
    خالی cutBasic(گراف_Acyc_راس* avertexp);
    خالی cutBackward(گراف_Acyc_راس* avertexp);
    خالی deleteMarked();
    خالی رکھو_2();
    خالی رکھو_Try_کنارہ(V3_گراف_کنارہ* edgep);
    منطقی رکھو_Iterate(گراف_Acyc_راس* vertexp, uint32_t currentRank);

    منطقی orig_Follow_کنارہ(V3_گراف_کنارہ* edgep) {
        واپس (edgep->وزن_2() && (m_orig_کنارہ_Funcp)(edgep));
    }
    خالی کنارہ_From_کنارہ(V3_گراف_کنارہ* oldedgep, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی) {
        // Make new breakGraph edge, with old edge as a template
        گراف_Acyc_کنارہ* ثابت newEdgep = نیا گراف_Acyc_کنارہ{&m_break_گراف, fromp, چوٹی,
                                                          oldedgep->وزن_2(), oldedgep->cutable()};
        newEdgep->userp(oldedgep->userp());  // Keep pointer to OrigEdgeList
    }
    خالی جوڑو_Orig_Edgep(V3_گراف_کنارہ* مقصد_Edgep, V3_گراف_کنارہ* جوڑو_Edgep) {
        // Add addEdge (or it's list) to list of edges that break edge represents
        // Note addEdge may already have a bunch of similar linked edge representations.  Yuk.
        UASSERT(جوڑو_Edgep, "Adding nullptr");
        اگر (!مقصد_Edgep->userp()) {
            Orig_کنارہ_List* ثابت oep = نیا Orig_کنارہ_List;
            m_orig_کنارہ_Delp.ڈالو_عقب(oep);
            مقصد_Edgep->userp(oep);
        }
        Orig_کنارہ_List* ثابت oEListp = static_cast<Orig_کنارہ_List*>(مقصد_Edgep->userp());
        اگر (Orig_کنارہ_List* ثابت جوڑو_Listp = static_cast<Orig_کنارہ_List*>(جوڑو_Edgep->userp())) {
            برائے (ثابت خودکار& itr : *جوڑو_Listp) oEListp->ڈالو_عقب(itr);
            جوڑو_Listp->clear();  // Done with it
        } ورنہ {
            oEListp->ڈالو_عقب(جوڑو_Edgep);
        }
    }
    خالی cut_Orig_کنارہ(V3_گراف_کنارہ* breakEdgep, ثابت حرف* why) {
        // From the break edge, cut edges in original graph it represents
        UINFO(8, why << " CUT " << breakEdgep->fromp());
        breakEdgep->cut();
        ثابت Orig_کنارہ_List* ثابت oEListp = static_cast<Orig_کنارہ_List*>(breakEdgep->userp());
        اگر (!oEListp) {
            v3fatalSrc("No original edge associated with cutting edge " << breakEdgep);
        }
        // The breakGraph edge may represent multiple real edges; cut them all
        برائے (ثابت خودکار& origEdgep : *oEListp) {
            origEdgep->cut();
            UINFO(8, "  " << why << "   " << origEdgep->fromp() << " ->" << origEdgep->چوٹی());
        }
    }
    // Work Queue
    خالی work_ڈالو(V3_گراف_راس* vertexp) {
        گراف_Acyc_راس* ثابت avertexp = static_cast<گراف_Acyc_راس*>(vertexp);
        // Add vertex to list of nodes needing further optimization trials
        اگر (!avertexp->m_onWorkList) {
            avertexp->m_onWorkList = سچ;
            m_work.link_عقب(avertexp);
        }
    }
    گراف_Acyc_راس* workBeginp() { واپس m_work.frontp(); }
    خالی work_نکالو() {
        گراف_Acyc_راس* ثابت avertexp = workBeginp();
        avertexp->m_onWorkList = جھوٹ;
        m_work.unlink(avertexp);
    }

عوامی:
    // CONSTRUCTORS
    گراف_Acyc(V3_گراف* origGraphp, V3_کنارہ_Func_P کنارہ_Funcp)
        : m_origGraphp{origGraphp}
        , m_orig_کنارہ_Funcp{کنارہ_Funcp} {}
    ~گراف_Acyc() {
        برائے (Orig_کنارہ_List* ip : m_orig_کنارہ_Delp) حذف ip;
        m_orig_کنارہ_Delp.clear();
    }
    خالی مین();
};

//--------------------------------------------------------------------

خالی گراف_Acyc::build_گراف(V3_گراف* origGraphp) {
    // Presumes the graph has been strongly ordered,
    // and thus there's a unique color if there are loops in this subgraph.

    // For each old node, make a new graph node for optimization
    origGraphp->userClearVertices();
    origGraphp->user_Clear_کنارے();
    برائے (V3_گراف_راس& overtex : origGraphp->vertices()) {
        اگر (overtex.color()) {
            گراف_Acyc_راس* ثابت avertexp = نیا گراف_Acyc_راس{&m_break_گراف, &overtex};
            overtex.userp(avertexp);  // Stash so can look up later
        }
    }

    // Build edges between logic vertices
    برائے (V3_گراف_راس& overtex : origGraphp->vertices()) {
        اگر (overtex.color()) {
            گراف_Acyc_راس* ثابت avertexp = static_cast<گراف_Acyc_راس*>(overtex.userp());
            build_گراف_Iterate(&overtex, avertexp);
        }
    }
}

خالی گراف_Acyc::build_گراف_Iterate(V3_گراف_راس* overtexp, گراف_Acyc_راس* avertexp) {
    // Make new edges
    برائے (V3_گراف_کنارہ& کنارہ : overtexp->برآمد_کنارے()) {
        اگر (orig_Follow_کنارہ(&کنارہ)) {  // not cut
            ثابت V3_گراف_راس* مقصد_Vertexp = کنارہ.چوٹی();
            اگر (مقصد_Vertexp->color()) {
                گراف_Acyc_راس* ثابت مقصد_AVertexp
                    = static_cast<گراف_Acyc_راس*>(مقصد_Vertexp->userp());
                // Replicate the old edge into the new graph
                // There may be multiple edges between same pairs of vertices
                V3_گراف_کنارہ* breakEdgep = نیا گراف_Acyc_کنارہ{&m_break_گراف, avertexp, مقصد_AVertexp,
                                                            کنارہ.وزن_2(), کنارہ.cutable()};
                جوڑو_Orig_Edgep(breakEdgep, &کنارہ);  // So can find original edge
            }
        }
    }
}

خالی گراف_Acyc::simplify(منطقی allowCut) {
    // Add all nodes to list of work to do
    برائے (V3_گراف_راس& راس : m_break_گراف.vertices()) work_ڈالو(&راس);
    // Optimize till everything finished
    جب (گراف_Acyc_راس* vertexp = workBeginp()) {
        work_نکالو();
        simplifyNone(vertexp);
        simplifyOne(vertexp);
        simplify_برآمد(vertexp);
        simplifyDup(vertexp);
        اگر (allowCut) {
            // The main algorithm works without these, though slower
            // So if changing the main algorithm, comment these out for a test run
            اگر (v3Global.opt.fAcycSimp()) {
                cutBasic(vertexp);
                cutBackward(vertexp);
            }
        }
    }
    deleteMarked();
}

خالی گراف_Acyc::deleteMarked() {
    // Delete nodes marked for removal
    برائے (V3_گراف_راس* ثابت vtxp : m_break_گراف.vertices().unlinkable()) {
        گراف_Acyc_راس* ثابت avertexp = static_cast<گراف_Acyc_راس*>(vtxp);
        اگر (avertexp->isDelete()) {
            VL_DO_DANGLING(avertexp->unlinkDelete(&m_break_گراف), avertexp);
        }
    }
}

خالی گراف_Acyc::simplifyNone(گراف_Acyc_راس* avertexp) {
    // Don't need any vertices with no inputs, There's no way they can have a loop.
    // Likewise, vertices with no outputs
    اگر (avertexp->isDelete()) واپس;
    اگر (avertexp->اندر_خالی_خانے() || avertexp->برآمد_خالی_خانے()) {
        UINFO(9, "  SimplifyNoneRemove " << avertexp);
        avertexp->سیٹ_Delete();  // Mark so we won't delete it twice
        // Remove edges
        جب (V3_گراف_کنارہ* ثابت edgep = avertexp->برآمد_کنارے().frontp()) {
            work_ڈالو(edgep->چوٹی());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        جب (V3_گراف_کنارہ* ثابت edgep = avertexp->اندر_کنارے().frontp()) {
            work_ڈالو(edgep->fromp());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
}

خالی گراف_Acyc::simplifyOne(گراف_Acyc_راس* avertexp) {
    // If a node has one input and one output, we can remove it and change the edges
    اگر (avertexp->isDelete()) واپس;
    اگر (avertexp->اندر_Size1() && avertexp->برآمد_Size1()) {
        V3_گراف_کنارہ* ثابت اندر_Edgep = avertexp->اندر_کنارے().frontp();
        V3_گراف_کنارہ* ثابت برآمد_Edgep = avertexp->برآمد_کنارے().frontp();
        V3_گراف_راس* ثابت اندر_Vertexp = اندر_Edgep->fromp();
        V3_گراف_راس* ثابت برآمد_Vertexp = برآمد_Edgep->چوٹی();
        // The in and out may be the same node; we'll make a loop
        // The in OR out may be THIS node; we can't delete it then.
        اگر (اندر_Vertexp != avertexp && برآمد_Vertexp != avertexp) {
            UINFO(9, "  SimplifyOneRemove " << avertexp);
            avertexp->سیٹ_Delete();  // Mark so we won't delete it twice
            // Make a new edge connecting the two vertices directly
            // If both are breakable, we pick the one with less weight, else it's arbitrary
            // We can forget about the origEdge list for the "non-selected" set of edges,
            // as we need to break only one set or the other set of edges, not both.
            // (This is why we must give preference to the cutable set.)
            V3_گراف_کنارہ* ثابت templateEdgep
                = ((اندر_Edgep->cutable()
                    && (!برآمد_Edgep->cutable() || اندر_Edgep->وزن_2() < برآمد_Edgep->وزن_2()))
                       ? اندر_Edgep
                       : برآمد_Edgep);
            // cppcheck-suppress leakReturnValNotUsed
            کنارہ_From_کنارہ(templateEdgep, اندر_Vertexp, برآمد_Vertexp);
            // Remove old edge
            VL_DO_DANGLING(اندر_Edgep->unlinkDelete(), اندر_Edgep);
            VL_DO_DANGLING(برآمد_Edgep->unlinkDelete(), برآمد_Edgep);
            VL_DANGLING(templateEdgep);
            work_ڈالو(اندر_Vertexp);
            work_ڈالو(برآمد_Vertexp);
        }
    }
}

خالی گراف_Acyc::simplify_برآمد(گراف_Acyc_راس* avertexp) {
    // If a node has one output that's not cutable, all its inputs can be reassigned
    // to the next node in the list
    اگر (avertexp->isDelete()) واپس;
    اگر (avertexp->برآمد_Size1()) {
        V3_گراف_کنارہ* ثابت برآمد_Edgep = avertexp->برآمد_کنارے().frontp();
        اگر (!برآمد_Edgep->cutable()) {
            V3_گراف_راس* برآمد_Vertexp = برآمد_Edgep->چوٹی();
            UINFO(9, "  SimplifyOutRemove " << avertexp);
            avertexp->سیٹ_Delete();  // Mark so we won't delete it twice
            برائے (V3_گراف_کنارہ* ثابت اندر_Edgep : avertexp->اندر_کنارے().unlinkable()) {
                V3_گراف_راس* اندر_Vertexp = اندر_Edgep->fromp();
                اگر (اندر_Vertexp == avertexp) {
                    اگر (debug()) v3error("Non-cutable vertex=" << avertexp);  // LCOV_EXCL_LINE
                    v3error("Circular logic when ordering code (non-cutable edge loop)\n"
                            << m_origGraphp->reportLoops(  // calls OrderGraph::loopsVertexCb
                                   &V3_گراف_کنارہ::followNotCutable, avertexp->origVertexp()));
                    // Things are unlikely to end well at this point,
                    // but we'll try something to get to further errors...
                    اندر_Edgep->cutable(سچ);
                    واپس;
                }
                // Make a new edge connecting the two vertices directly
                // cppcheck-suppress leakReturnValNotUsed
                کنارہ_From_کنارہ(اندر_Edgep, اندر_Vertexp, برآمد_Vertexp);
                // Remove old edge
                VL_DO_DANGLING(اندر_Edgep->unlinkDelete(), اندر_Edgep);
                work_ڈالو(اندر_Vertexp);
            }
            VL_DO_DANGLING(برآمد_Edgep->unlinkDelete(), برآمد_Edgep);
            work_ڈالو(برآمد_Vertexp);
        }
    }
}

خالی گراف_Acyc::simplifyDup(گراف_Acyc_راس* avertexp) {
    // Remove redundant edges
    اگر (avertexp->isDelete()) واپس;
    // Clear marks
    برائے (V3_گراف_کنارہ& کنارہ : avertexp->برآمد_کنارے()) کنارہ.چوٹی()->userp(خالی_اشاریہ);
    // Mark edges and detect duplications
    برائے (V3_گراف_کنارہ* ثابت edgep : avertexp->برآمد_کنارے().unlinkable()) {
        V3_گراف_راس* برآمد_Vertexp = edgep->چوٹی();
        V3_گراف_کنارہ* پچھلا_Edgep = static_cast<V3_گراف_کنارہ*>(برآمد_Vertexp->userp());
        اگر (پچھلا_Edgep) {
            اگر (!پچھلا_Edgep->cutable()) {
                // !cutable duplicates prev !cutable: we can ignore it, redundant
                //  cutable duplicates prev !cutable: know it's not a relevant loop, ignore it
                UINFO(8, "    DelDupEdge " << avertexp << " -> " << edgep->چوٹی());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            } ورنہ اگر (!edgep->cutable()) {
                // !cutable duplicates prev  cutable: delete the earlier cutable
                UINFO(8, "    DelDupPrev " << avertexp << " -> " << پچھلا_Edgep->چوٹی());
                VL_DO_DANGLING(پچھلا_Edgep->unlinkDelete(), پچھلا_Edgep);
                برآمد_Vertexp->userp(edgep);
            } ورنہ {
                //  cutable duplicates prev  cutable: combine weights
                UINFO(8, "    DelDupComb " << avertexp << " -> " << edgep->چوٹی());
                پچھلا_Edgep->وزن_2(پچھلا_Edgep->وزن_2() + edgep->وزن_2());
                جوڑو_Orig_Edgep(پچھلا_Edgep, edgep);
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            work_ڈالو(برآمد_Vertexp);
            work_ڈالو(avertexp);
        } ورنہ {
            // No previous assignment
            برآمد_Vertexp->userp(edgep);
        }
    }
}

خالی گراف_Acyc::cutBasic(گراف_Acyc_راس* avertexp) {
    // Detect and cleanup any loops from node to itself
    اگر (avertexp->isDelete()) واپس;
    برائے (V3_گراف_کنارہ* ثابت edgep : avertexp->برآمد_کنارے().unlinkable()) {
        اگر (edgep->cutable() && edgep->چوٹی() == avertexp) {
            cut_Orig_کنارہ(edgep, "  Cut Basic");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_ڈالو(avertexp);
        }
    }
}

خالی گراف_Acyc::cutBackward(گراف_Acyc_راس* avertexp) {
    // If a cutable edge is from A->B, and there's a non-cutable edge B->A, then must cut!
    اگر (avertexp->isDelete()) واپس;
    // Clear marks
    برائے (V3_گراف_کنارہ& کنارہ : avertexp->برآمد_کنارے()) کنارہ.چوٹی()->user(جھوٹ);
    برائے (V3_گراف_کنارہ& کنارہ : avertexp->اندر_کنارے()) {
        اگر (!کنارہ.cutable()) کنارہ.fromp()->user(سچ);
    }
    // Detect duplications
    برائے (V3_گراف_کنارہ* ثابت edgep : avertexp->برآمد_کنارے().unlinkable()) {
        اگر (edgep->cutable() && edgep->چوٹی()->user()) {
            cut_Orig_کنارہ(edgep, "  Cut A->B->A");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_ڈالو(avertexp);
        }
    }
}

خالی گراف_Acyc::رکھو_2() {
    // Input is m_breakGraph with ranks already assigned on non-breakable edges

    // Make a list of all cutable edges in the graph
    صحیح عدد_کنارے = 0;
    برائے (V3_گراف_راس& راس : m_break_گراف.vertices()) {
        برائے (V3_گراف_کنارہ& کنارہ : راس.برآمد_کنارے()) {
            اگر (کنارہ.وزن_2() && کنارہ.cutable()) ++عدد_کنارے;
        }
    }
    UINFO(4, "    Cutable edges = " << عدد_کنارے);

    std::سمتیہ<V3_گراف_کنارہ*> کنارے;  // List of all edges to be processed
    // Make the vector properly sized right off the bat -- faster than reallocating
    کنارے.reserve(عدد_کنارے + 1);
    برائے (V3_گراف_راس& راس : m_break_گراف.vertices()) {
        راس.user(0);  // Clear in prep of next step
        برائے (V3_گراف_کنارہ& کنارہ : راس.برآمد_کنارے()) {
            اگر (کنارہ.وزن_2() && کنارہ.cutable()) کنارے.ڈالو_عقب(&کنارہ);
        }
    }

    // Sort by weight, then by vertex (so that we completely process one vertex, when possible)
    stable_ترتیب_دو(کنارے.begin(), کنارے.انتہا(), گراف_Acyc_کنارہ_Cmp());

    // Process each edge in weighted order
    m_رکھو_قدم = 10;
    برائے (V3_گراف_کنارہ* edgep : کنارے) رکھو_Try_کنارہ(edgep);
}

خالی گراف_Acyc::رکھو_Try_کنارہ(V3_گراف_کنارہ* edgep) {
    // Try to make this edge uncutable
    m_رکھو_قدم++;
    UINFO(8, "    PlaceEdge s" << m_رکھو_قدم << " w" << edgep->وزن_2() << " " << edgep->fromp());
    // Make the edge uncutable so we detect it in placement
    edgep->cutable(جھوٹ);
    // Vertex::m_user begin: number indicates this edge was completed
    // Try to assign ranks, presuming this edge is in place
    // If we come across user()==placestep, we've detected a loop and must back out
    ثابت منطقی loop
        = رکھو_Iterate(static_cast<گراف_Acyc_راس*>(edgep->چوٹی()), edgep->fromp()->rank() + 1);
    اگر (!loop) {
        // No loop, we can keep it as uncutable
        // Commit the new ranks we calculated
        // Just cleanup the list.  If this is slow, we can add another set of
        // user counters to avoid cleaning up the list.
        جب (workBeginp()) work_نکالو();
    } ورنہ {
        // Adding this edge would cause a loop, kill it
        edgep->cutable(سچ);  // So graph still looks pretty
        cut_Orig_کنارہ(edgep, "  Cut loop");
        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        // Back out the ranks we calculated
        جب (گراف_Acyc_راس* vertexp = workBeginp()) {
            work_نکالو();
            vertexp->rank(vertexp->m_storedRank);
        }
    }
}

منطقی گراف_Acyc::رکھو_Iterate(گراف_Acyc_راس* vertexp, uint32_t currentRank) {
    // Assign rank to each unvisited node
    //   rank() is the "committed rank" of the graph known without loops
    // If larger rank is found, assign it and loop back through
    // If we hit a back node make a list of all loops
    اگر (vertexp->rank() >= currentRank) واپس جھوٹ;  // Already processed it
    اگر (vertexp->user() == m_رکھو_قدم) واپس سچ;  // Loop detected
    vertexp->user(m_رکھو_قدم);
    // Remember we're changing the rank of this node; might need to back out
    اگر (!vertexp->m_onWorkList) {
        vertexp->m_storedRank = vertexp->rank();
        work_ڈالو(vertexp);
    }
    vertexp->rank(currentRank);
    // Follow all edges and increase their ranks
    برائے (V3_گراف_کنارہ& کنارہ : vertexp->برآمد_کنارے()) {
        اگر (کنارہ.وزن_2() && !کنارہ.cutable()) {
            اگر (رکھو_Iterate(static_cast<گراف_Acyc_راس*>(کنارہ.چوٹی()), currentRank + 1)) {
                // We don't need to reset user(); we'll use a different placeStep for the next edge
                واپس سچ;  // Loop detected
            }
        }
    }
    vertexp->user(0);
    واپس جھوٹ;
}

//----- Main algorithm entry point

خالی گراف_Acyc::مین() {
    m_break_گراف.user_Clear_کنارے();

    // Color based on possible loops
    m_origGraphp->stronglyConnected(m_orig_کنارہ_Funcp);

    // Make a new graph with vertices that have only a single vertex
    // for each group of old vertices that are interconnected with unbreakable
    // edges (and thus can't represent loops - if we did the unbreakable
    // marking right, anyways)
    build_گراف(m_origGraphp);
    اگر (dump_گراف_Level() >= 6) m_break_گراف.dumpDotFilePrefixed("acyc_pre");

    // Perform simple optimizations before any cuttings
    simplify(جھوٹ);
    اگر (dump_گراف_Level() >= 5) m_break_گراف.dumpDotFilePrefixed("acyc_simp");

    UINFO(4, " Cutting trivial loops");
    simplify(سچ);
    اگر (dump_گراف_Level() >= 6) m_break_گراف.dumpDotFilePrefixed("acyc_mid");

    UINFO(4, " Ranking");
    m_break_گراف.rank(&V3_گراف_کنارہ::followNotCutable);
    اگر (dump_گراف_Level() >= 6) m_break_گراف.dumpDotFilePrefixed("acyc_rank");

    UINFO(4, " Placement");
    رکھو_2();
    اگر (dump_گراف_Level() >= 6) m_break_گراف.dumpDotFilePrefixed("acyc_place");

    UINFO(4, " Final Ranking");
    // Only needed to assert there are no loops in completed graph
    m_break_گراف.rank(&V3_گراف_کنارہ::followAlwaysTrue);
    اگر (dump_گراف_Level() >= 6) m_break_گراف.dumpDotFilePrefixed("acyc_done");
}

خالی V3_گراف::acyclic(V3_کنارہ_Func_P کنارہ_Funcp) {
    UINFO(4, "Acyclic");
    گراف_Acyc acyc{یہ, کنارہ_Funcp};
    acyc.مین();
    UINFO(4, "Acyclic done");
}
