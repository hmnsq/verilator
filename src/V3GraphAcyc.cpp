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

#define VL_MT_DISABLED_CODE_UNIT 1

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

ವರ್ಗ GraphAcycVertex final : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(GraphAcycVertex, V3GraphVertex)
    // user() is used for various sub-algorithm pieces
    V3GraphVertex* ಧ್ರುವ m_origVertexp;  // Pointer to first vertex this represents
ರಕ್ಷಿತ:
    friend ವರ್ಗ GraphAcyc;
    V3ListLinks<GraphAcycVertex> m_links;  // List links to store instances of this class
    uint32_t m_storedRank = 0;  // Rank held until commit to edge placement
    ತಾರ್ಕಿಕ m_onWorkList = ಅಸತ್ಯ;  // True if already on list of work to do
    ತಾರ್ಕಿಕ m_deleted = ಅಸತ್ಯ;  // True if deleted

ಗುಪ್ತ:
    V3ListLinks<GraphAcycVertex>& links() { ಪ್ರತ್ಯಾಗಮ m_links; }

ಸಾರ್ವಜನಿಕ:
    // List type to store instances of this class
    using List = V3List<GraphAcycVertex, &GraphAcycVertex::links>;

    GraphAcycVertex(V3Graph* graphp, V3GraphVertex* origVertexp)
        : V3GraphVertex{graphp}
        , m_origVertexp{origVertexp} {}
    ~GraphAcycVertex() override = ಸ್ವಭಾವ;
    V3GraphVertex* origVertexp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_origVertexp; }
    ಶೂನ್ಯ setDelete() { m_deleted = ಸತ್ಯ; }
    ತಾರ್ಕಿಕ isDelete() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_deleted; }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string name() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_origVertexp->name(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_origVertexp->dotColor(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    File_ಸಾಲು* fileline() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_origVertexp->fileline(); }
};

//--------------------------------------------------------------------

ವರ್ಗ GraphAcycEdge final : ಸಾರ್ವಜನಿಕ V3GraphEdge {
    VL_RTTI_IMPL(GraphAcycEdge, V3GraphEdge)
    // userp() is always used to point to the head original graph edge
ಗುಪ್ತ:
    using OrigEdgeList = std::list<V3GraphEdge*>;  // List of orig edges, see also GraphAcyc's decl
    V3GraphEdge* origEdgep() ಧ್ರುವ {
        ಧ್ರುವ OrigEdgeList* ಧ್ರುವ oEListp = static_cast<OrigEdgeList*>(userp());
        UASSERT(oEListp, "No original edge associated with acyc edge " << ಏತತ್);
        ಪ್ರತ್ಯಾಗಮ (oEListp->front());
    }

ಸಾರ್ವಜನಿಕ:
    GraphAcycEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ತುದಿ, ಪೂರ್ಣಾಙ್ಕ weight,
                  ತಾರ್ಕಿಕ cutable = ಅಸತ್ಯ)
        : V3GraphEdge{graphp, fromp, ತುದಿ, weight, cutable} {}
    ~GraphAcycEdge() override = ಸ್ವಭಾವ;
    // yellow=we might still cut it, else oldEdge: yellowGreen=made uncutable, red=uncutable
    string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ (cutable() ? "yellow" : origEdgep()->dotColor()); }
};

//--------------------------------------------------------------------

ಸಂರಚನಾ GraphAcycEdgeCmp final {
    ತಾರ್ಕಿಕ operator()(ಧ್ರುವ V3GraphEdge* lhsp, ಧ್ರುವ V3GraphEdge* rhsp) ಧ್ರುವ {
        ಯದಿ (lhsp->weight() > rhsp->weight()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // LHS goes first
        ಯದಿ (lhsp->weight() < rhsp->weight()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // RHS goes first
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
};

//--------------------------------------------------------------------

// CLASSES
ವರ್ಗ GraphAcyc final {
    using OrigEdgeList
        = std::list<V3GraphEdge*>;  // List of orig edges, see also GraphAcycEdge's decl
    // GRAPH USERS
    //  origGraph
    //    GraphVertex::user()   GraphAycVerted* New graph node
    //  m_breakGraph
    //    GraphEdge::user()     OrigEdgeList*   Old graph edges
    //    GraphVertex::user     bool            Detection of loops in simplifyDupIterate
    // MEMBERS
    V3Graph* ಧ್ರುವ m_origGraphp;  // Original graph
    V3Graph m_breakGraph;  // Graph with only breakable edges represented
    GraphAcycVertex::List m_work;  // List of vertices with optimization work left
    std::vector<OrigEdgeList*> m_origEdgeDelp;  // List of deletions to do when done
    ಧ್ರುವ V3EdgeFuncP
        m_origEdgeFuncp;  // Function that says we follow this edge (in original graph)
    uint32_t m_placeStep = 0;  // Number that user() must be equal to to indicate processing

    // METHODS
    ಶೂನ್ಯ buildGraph(V3Graph* origGraphp);
    ಶೂನ್ಯ buildGraphIterate(V3GraphVertex* overtexp, GraphAcycVertex* avertexp);
    ಶೂನ್ಯ simplify(ತಾರ್ಕಿಕ allowCut);
    ಶೂನ್ಯ simplifyNone(GraphAcycVertex* avertexp);
    ಶೂನ್ಯ simplifyOne(GraphAcycVertex* avertexp);
    ಶೂನ್ಯ simplifyOut(GraphAcycVertex* avertexp);
    ಶೂನ್ಯ simplifyDup(GraphAcycVertex* avertexp);
    ಶೂನ್ಯ cutBasic(GraphAcycVertex* avertexp);
    ಶೂನ್ಯ cutBackward(GraphAcycVertex* avertexp);
    ಶೂನ್ಯ deleteMarked();
    ಶೂನ್ಯ place();
    ಶೂನ್ಯ placeTryEdge(V3GraphEdge* edgep);
    ತಾರ್ಕಿಕ placeIterate(GraphAcycVertex* vertexp, uint32_t currentRank);

    ತಾರ್ಕಿಕ origFollowEdge(V3GraphEdge* edgep) {
        ಪ್ರತ್ಯಾಗಮ (edgep->weight() && (m_origEdgeFuncp)(edgep));
    }
    ಶೂನ್ಯ edgeFromEdge(V3GraphEdge* oldedgep, V3GraphVertex* fromp, V3GraphVertex* ತುದಿ) {
        // Make new breakGraph edge, with old edge as a template
        GraphAcycEdge* ಧ್ರುವ newEdgep = ನೂತನ GraphAcycEdge{&m_breakGraph, fromp, ತುದಿ,
                                                          oldedgep->weight(), oldedgep->cutable()};
        newEdgep->userp(oldedgep->userp());  // Keep pointer to OrigEdgeList
    }
    ಶೂನ್ಯ addOrigEdgep(V3GraphEdge* toEdgep, V3GraphEdge* addEdgep) {
        // Add addEdge (or it's list) to list of edges that break edge represents
        // Note addEdge may already have a bunch of similar linked edge representations.  Yuk.
        UASSERT(addEdgep, "Adding nullptr");
        ಯದಿ (!toEdgep->userp()) {
            OrigEdgeList* ಧ್ರುವ oep = ನೂತನ OrigEdgeList;
            m_origEdgeDelp.ತಳ್ಳು_back(oep);
            toEdgep->userp(oep);
        }
        OrigEdgeList* ಧ್ರುವ oEListp = static_cast<OrigEdgeList*>(toEdgep->userp());
        ಯದಿ (OrigEdgeList* ಧ್ರುವ addListp = static_cast<OrigEdgeList*>(addEdgep->userp())) {
            ಪ್ರತಿ (ಧ್ರುವ auto& itr : *addListp) oEListp->ತಳ್ಳು_back(itr);
            addListp->clear();  // Done with it
        } ಅನ್ಯಥಾ {
            oEListp->ತಳ್ಳು_back(addEdgep);
        }
    }
    ಶೂನ್ಯ cutOrigEdge(V3GraphEdge* breakEdgep, ಧ್ರುವ ವರ್ಣ* why) {
        // From the break edge, cut edges in original graph it represents
        UINFO(8, why << " CUT " << breakEdgep->fromp());
        breakEdgep->cut();
        ಧ್ರುವ OrigEdgeList* ಧ್ರುವ oEListp = static_cast<OrigEdgeList*>(breakEdgep->userp());
        ಯದಿ (!oEListp) {
            v3fatalSrc("No original edge associated with cutting edge " << breakEdgep);
        }
        // The breakGraph edge may represent multiple real edges; cut them all
        ಪ್ರತಿ (ಧ್ರುವ auto& origEdgep : *oEListp) {
            origEdgep->cut();
            UINFO(8, "  " << why << "   " << origEdgep->fromp() << " ->" << origEdgep->ತುದಿ());
        }
    }
    // Work Queue
    ಶೂನ್ಯ work_ತಳ್ಳು(V3GraphVertex* vertexp) {
        GraphAcycVertex* ಧ್ರುವ avertexp = static_cast<GraphAcycVertex*>(vertexp);
        // Add vertex to list of nodes needing further optimization trials
        ಯದಿ (!avertexp->m_onWorkList) {
            avertexp->m_onWorkList = ಸತ್ಯ;
            m_work.linkBack(avertexp);
        }
    }
    GraphAcycVertex* workBeginp() { ಪ್ರತ್ಯಾಗಮ m_work.frontp(); }
    ಶೂನ್ಯ work_ತೆಗೆ() {
        GraphAcycVertex* ಧ್ರುವ avertexp = workBeginp();
        avertexp->m_onWorkList = ಅಸತ್ಯ;
        m_work.unlink(avertexp);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    GraphAcyc(V3Graph* origGraphp, V3EdgeFuncP edgeFuncp)
        : m_origGraphp{origGraphp}
        , m_origEdgeFuncp{edgeFuncp} {}
    ~GraphAcyc() {
        ಪ್ರತಿ (OrigEdgeList* ip : m_origEdgeDelp) delete ip;
        m_origEdgeDelp.clear();
    }
    ಶೂನ್ಯ main();
};

//--------------------------------------------------------------------

ಶೂನ್ಯ GraphAcyc::buildGraph(V3Graph* origGraphp) {
    // Presumes the graph has been strongly ordered,
    // and thus there's a unique color if there are loops in this subgraph.

    // For each old node, make a new graph node for optimization
    origGraphp->userClearVertices();
    origGraphp->userClearEdges();
    ಪ್ರತಿ (V3GraphVertex& overtex : origGraphp->vertices()) {
        ಯದಿ (overtex.color()) {
            GraphAcycVertex* ಧ್ರುವ avertexp = ನೂತನ GraphAcycVertex{&m_breakGraph, &overtex};
            overtex.userp(avertexp);  // Stash so can look up later
        }
    }

    // Build edges between logic vertices
    ಪ್ರತಿ (V3GraphVertex& overtex : origGraphp->vertices()) {
        ಯದಿ (overtex.color()) {
            GraphAcycVertex* ಧ್ರುವ avertexp = static_cast<GraphAcycVertex*>(overtex.userp());
            buildGraphIterate(&overtex, avertexp);
        }
    }
}

ಶೂನ್ಯ GraphAcyc::buildGraphIterate(V3GraphVertex* overtexp, GraphAcycVertex* avertexp) {
    // Make new edges
    ಪ್ರತಿ (V3GraphEdge& edge : overtexp->outEdges()) {
        ಯದಿ (origFollowEdge(&edge)) {  // not cut
            ಧ್ರುವ V3GraphVertex* toVertexp = edge.ತುದಿ();
            ಯದಿ (toVertexp->color()) {
                GraphAcycVertex* ಧ್ರುವ toAVertexp
                    = static_cast<GraphAcycVertex*>(toVertexp->userp());
                // Replicate the old edge into the new graph
                // There may be multiple edges between same pairs of vertices
                V3GraphEdge* breakEdgep = ನೂತನ GraphAcycEdge{&m_breakGraph, avertexp, toAVertexp,
                                                            edge.weight(), edge.cutable()};
                addOrigEdgep(breakEdgep, &edge);  // So can find original edge
            }
        }
    }
}

ಶೂನ್ಯ GraphAcyc::simplify(ತಾರ್ಕಿಕ allowCut) {
    // Add all nodes to list of work to do
    ಪ್ರತಿ (V3GraphVertex& vertex : m_breakGraph.vertices()) work_ತಳ್ಳು(&vertex);
    // Optimize till everything finished
    ಯಾವತ್ (GraphAcycVertex* vertexp = workBeginp()) {
        work_ತೆಗೆ();
        simplifyNone(vertexp);
        simplifyOne(vertexp);
        simplifyOut(vertexp);
        simplifyDup(vertexp);
        ಯದಿ (allowCut) {
            // The main algorithm works without these, though slower
            // So if changing the main algorithm, comment these out for a test run
            ಯದಿ (v3Global.opt.fAcycSimp()) {
                cutBasic(vertexp);
                cutBackward(vertexp);
            }
        }
    }
    deleteMarked();
}

ಶೂನ್ಯ GraphAcyc::deleteMarked() {
    // Delete nodes marked for removal
    ಪ್ರತಿ (V3GraphVertex* ಧ್ರುವ vtxp : m_breakGraph.vertices().unlinkable()) {
        GraphAcycVertex* ಧ್ರುವ avertexp = static_cast<GraphAcycVertex*>(vtxp);
        ಯದಿ (avertexp->isDelete()) {
            VL_DO_DANGLING(avertexp->unlinkDelete(&m_breakGraph), avertexp);
        }
    }
}

ಶೂನ್ಯ GraphAcyc::simplifyNone(GraphAcycVertex* avertexp) {
    // Don't need any vertices with no inputs, There's no way they can have a loop.
    // Likewise, vertices with no outputs
    ಯದಿ (avertexp->isDelete()) ಪ್ರತ್ಯಾಗಮ;
    ಯದಿ (avertexp->inEmpty() || avertexp->outEmpty()) {
        UINFO(9, "  SimplifyNoneRemove " << avertexp);
        avertexp->setDelete();  // Mark so we won't delete it twice
        // Remove edges
        ಯಾವತ್ (V3GraphEdge* ಧ್ರುವ edgep = avertexp->outEdges().frontp()) {
            work_ತಳ್ಳು(edgep->ತುದಿ());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        ಯಾವತ್ (V3GraphEdge* ಧ್ರುವ edgep = avertexp->inEdges().frontp()) {
            work_ತಳ್ಳು(edgep->fromp());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
}

ಶೂನ್ಯ GraphAcyc::simplifyOne(GraphAcycVertex* avertexp) {
    // If a node has one input and one output, we can remove it and change the edges
    ಯದಿ (avertexp->isDelete()) ಪ್ರತ್ಯಾಗಮ;
    ಯದಿ (avertexp->inSize1() && avertexp->outSize1()) {
        V3GraphEdge* ಧ್ರುವ inEdgep = avertexp->inEdges().frontp();
        V3GraphEdge* ಧ್ರುವ outEdgep = avertexp->outEdges().frontp();
        V3GraphVertex* ಧ್ರುವ inVertexp = inEdgep->fromp();
        V3GraphVertex* ಧ್ರುವ outVertexp = outEdgep->ತುದಿ();
        // The in and out may be the same node; we'll make a loop
        // The in OR out may be THIS node; we can't delete it then.
        ಯದಿ (inVertexp != avertexp && outVertexp != avertexp) {
            UINFO(9, "  SimplifyOneRemove " << avertexp);
            avertexp->setDelete();  // Mark so we won't delete it twice
            // Make a new edge connecting the two vertices directly
            // If both are breakable, we pick the one with less weight, else it's arbitrary
            // We can forget about the origEdge list for the "non-selected" set of edges,
            // as we need to break only one set or the other set of edges, not both.
            // (This is why we must give preference to the cutable set.)
            V3GraphEdge* ಧ್ರುವ templateEdgep
                = ((inEdgep->cutable()
                    && (!outEdgep->cutable() || inEdgep->weight() < outEdgep->weight()))
                       ? inEdgep
                       : outEdgep);
            // cppcheck-suppress leakReturnValNotUsed
            edgeFromEdge(templateEdgep, inVertexp, outVertexp);
            // Remove old edge
            VL_DO_DANGLING(inEdgep->unlinkDelete(), inEdgep);
            VL_DO_DANGLING(outEdgep->unlinkDelete(), outEdgep);
            VL_DANGLING(templateEdgep);
            work_ತಳ್ಳು(inVertexp);
            work_ತಳ್ಳು(outVertexp);
        }
    }
}

ಶೂನ್ಯ GraphAcyc::simplifyOut(GraphAcycVertex* avertexp) {
    // If a node has one output that's not cutable, all its inputs can be reassigned
    // to the next node in the list
    ಯದಿ (avertexp->isDelete()) ಪ್ರತ್ಯಾಗಮ;
    ಯದಿ (avertexp->outSize1()) {
        V3GraphEdge* ಧ್ರುವ outEdgep = avertexp->outEdges().frontp();
        ಯದಿ (!outEdgep->cutable()) {
            V3GraphVertex* outVertexp = outEdgep->ತುದಿ();
            UINFO(9, "  SimplifyOutRemove " << avertexp);
            avertexp->setDelete();  // Mark so we won't delete it twice
            ಪ್ರತಿ (V3GraphEdge* ಧ್ರುವ inEdgep : avertexp->inEdges().unlinkable()) {
                V3GraphVertex* inVertexp = inEdgep->fromp();
                ಯದಿ (inVertexp == avertexp) {
                    ಯದಿ (debug()) v3error("Non-cutable vertex=" << avertexp);  // LCOV_EXCL_LINE
                    v3error("Circular logic when ordering code (non-cutable edge loop)\n"
                            << m_origGraphp->reportLoops(  // calls OrderGraph::loopsVertexCb
                                   &V3GraphEdge::followNotCutable, avertexp->origVertexp()));
                    // Things are unlikely to end well at this point,
                    // but we'll try something to get to further errors...
                    inEdgep->cutable(ಸತ್ಯ);
                    ಪ್ರತ್ಯಾಗಮ;
                }
                // Make a new edge connecting the two vertices directly
                // cppcheck-suppress leakReturnValNotUsed
                edgeFromEdge(inEdgep, inVertexp, outVertexp);
                // Remove old edge
                VL_DO_DANGLING(inEdgep->unlinkDelete(), inEdgep);
                work_ತಳ್ಳು(inVertexp);
            }
            VL_DO_DANGLING(outEdgep->unlinkDelete(), outEdgep);
            work_ತಳ್ಳು(outVertexp);
        }
    }
}

ಶೂನ್ಯ GraphAcyc::simplifyDup(GraphAcycVertex* avertexp) {
    // Remove redundant edges
    ಯದಿ (avertexp->isDelete()) ಪ್ರತ್ಯಾಗಮ;
    // Clear marks
    ಪ್ರತಿ (V3GraphEdge& edge : avertexp->outEdges()) edge.ತುದಿ()->userp(nullptr);
    // Mark edges and detect duplications
    ಪ್ರತಿ (V3GraphEdge* ಧ್ರುವ edgep : avertexp->outEdges().unlinkable()) {
        V3GraphVertex* outVertexp = edgep->ತುದಿ();
        V3GraphEdge* prevEdgep = static_cast<V3GraphEdge*>(outVertexp->userp());
        ಯದಿ (prevEdgep) {
            ಯದಿ (!prevEdgep->cutable()) {
                // !cutable duplicates prev !cutable: we can ignore it, redundant
                //  cutable duplicates prev !cutable: know it's not a relevant loop, ignore it
                UINFO(8, "    DelDupEdge " << avertexp << " -> " << edgep->ತುದಿ());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            } ಅನ್ಯಥಾ ಯದಿ (!edgep->cutable()) {
                // !cutable duplicates prev  cutable: delete the earlier cutable
                UINFO(8, "    DelDupPrev " << avertexp << " -> " << prevEdgep->ತುದಿ());
                VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                outVertexp->userp(edgep);
            } ಅನ್ಯಥಾ {
                //  cutable duplicates prev  cutable: combine weights
                UINFO(8, "    DelDupComb " << avertexp << " -> " << edgep->ತುದಿ());
                prevEdgep->weight(prevEdgep->weight() + edgep->weight());
                addOrigEdgep(prevEdgep, edgep);
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            work_ತಳ್ಳು(outVertexp);
            work_ತಳ್ಳು(avertexp);
        } ಅನ್ಯಥಾ {
            // No previous assignment
            outVertexp->userp(edgep);
        }
    }
}

ಶೂನ್ಯ GraphAcyc::cutBasic(GraphAcycVertex* avertexp) {
    // Detect and cleanup any loops from node to itself
    ಯದಿ (avertexp->isDelete()) ಪ್ರತ್ಯಾಗಮ;
    ಪ್ರತಿ (V3GraphEdge* ಧ್ರುವ edgep : avertexp->outEdges().unlinkable()) {
        ಯದಿ (edgep->cutable() && edgep->ತುದಿ() == avertexp) {
            cutOrigEdge(edgep, "  Cut Basic");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_ತಳ್ಳು(avertexp);
        }
    }
}

ಶೂನ್ಯ GraphAcyc::cutBackward(GraphAcycVertex* avertexp) {
    // If a cutable edge is from A->B, and there's a non-cutable edge B->A, then must cut!
    ಯದಿ (avertexp->isDelete()) ಪ್ರತ್ಯಾಗಮ;
    // Clear marks
    ಪ್ರತಿ (V3GraphEdge& edge : avertexp->outEdges()) edge.ತುದಿ()->user(ಅಸತ್ಯ);
    ಪ್ರತಿ (V3GraphEdge& edge : avertexp->inEdges()) {
        ಯದಿ (!edge.cutable()) edge.fromp()->user(ಸತ್ಯ);
    }
    // Detect duplications
    ಪ್ರತಿ (V3GraphEdge* ಧ್ರುವ edgep : avertexp->outEdges().unlinkable()) {
        ಯದಿ (edgep->cutable() && edgep->ತುದಿ()->user()) {
            cutOrigEdge(edgep, "  Cut A->B->A");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_ತಳ್ಳು(avertexp);
        }
    }
}

ಶೂನ್ಯ GraphAcyc::place() {
    // Input is m_breakGraph with ranks already assigned on non-breakable edges

    // Make a list of all cutable edges in the graph
    ಪೂರ್ಣಾಙ್ಕ ಅಂಕೆ_Edges = 0;
    ಪ್ರತಿ (V3GraphVertex& vertex : m_breakGraph.vertices()) {
        ಪ್ರತಿ (V3GraphEdge& edge : vertex.outEdges()) {
            ಯದಿ (edge.weight() && edge.cutable()) ++ಅಂಕೆ_Edges;
        }
    }
    UINFO(4, "    Cutable edges = " << ಅಂಕೆ_Edges);

    std::vector<V3GraphEdge*> edges;  // List of all edges to be processed
    // Make the vector properly sized right off the bat -- faster than reallocating
    edges.reserve(ಅಂಕೆ_Edges + 1);
    ಪ್ರತಿ (V3GraphVertex& vertex : m_breakGraph.vertices()) {
        vertex.user(0);  // Clear in prep of next step
        ಪ್ರತಿ (V3GraphEdge& edge : vertex.outEdges()) {
            ಯದಿ (edge.weight() && edge.cutable()) edges.ತಳ್ಳು_back(&edge);
        }
    }

    // Sort by weight, then by vertex (so that we completely process one vertex, when possible)
    stable_sort(edges.begin(), edges.end(), GraphAcycEdgeCmp());

    // Process each edge in weighted order
    m_placeStep = 10;
    ಪ್ರತಿ (V3GraphEdge* edgep : edges) placeTryEdge(edgep);
}

ಶೂನ್ಯ GraphAcyc::placeTryEdge(V3GraphEdge* edgep) {
    // Try to make this edge uncutable
    m_placeStep++;
    UINFO(8, "    PlaceEdge s" << m_placeStep << " w" << edgep->weight() << " " << edgep->fromp());
    // Make the edge uncutable so we detect it in placement
    edgep->cutable(ಅಸತ್ಯ);
    // Vertex::m_user begin: number indicates this edge was completed
    // Try to assign ranks, presuming this edge is in place
    // If we come across user()==placestep, we've detected a loop and must back out
    ಧ್ರುವ ತಾರ್ಕಿಕ loop
        = placeIterate(static_cast<GraphAcycVertex*>(edgep->ತುದಿ()), edgep->fromp()->rank() + 1);
    ಯದಿ (!loop) {
        // No loop, we can keep it as uncutable
        // Commit the new ranks we calculated
        // Just cleanup the list.  If this is slow, we can add another set of
        // user counters to avoid cleaning up the list.
        ಯಾವತ್ (workBeginp()) work_ತೆಗೆ();
    } ಅನ್ಯಥಾ {
        // Adding this edge would cause a loop, kill it
        edgep->cutable(ಸತ್ಯ);  // So graph still looks pretty
        cutOrigEdge(edgep, "  Cut loop");
        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        // Back out the ranks we calculated
        ಯಾವತ್ (GraphAcycVertex* vertexp = workBeginp()) {
            work_ತೆಗೆ();
            vertexp->rank(vertexp->m_storedRank);
        }
    }
}

ತಾರ್ಕಿಕ GraphAcyc::placeIterate(GraphAcycVertex* vertexp, uint32_t currentRank) {
    // Assign rank to each unvisited node
    //   rank() is the "committed rank" of the graph known without loops
    // If larger rank is found, assign it and loop back through
    // If we hit a back node make a list of all loops
    ಯದಿ (vertexp->rank() >= currentRank) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Already processed it
    ಯದಿ (vertexp->user() == m_placeStep) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // Loop detected
    vertexp->user(m_placeStep);
    // Remember we're changing the rank of this node; might need to back out
    ಯದಿ (!vertexp->m_onWorkList) {
        vertexp->m_storedRank = vertexp->rank();
        work_ತಳ್ಳು(vertexp);
    }
    vertexp->rank(currentRank);
    // Follow all edges and increase their ranks
    ಪ್ರತಿ (V3GraphEdge& edge : vertexp->outEdges()) {
        ಯದಿ (edge.weight() && !edge.cutable()) {
            ಯದಿ (placeIterate(static_cast<GraphAcycVertex*>(edge.ತುದಿ()), currentRank + 1)) {
                // We don't need to reset user(); we'll use a different placeStep for the next edge
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // Loop detected
            }
        }
    }
    vertexp->user(0);
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
}

//----- Main algorithm entry point

ಶೂನ್ಯ GraphAcyc::main() {
    m_breakGraph.userClearEdges();

    // Color based on possible loops
    m_origGraphp->stronglyConnected(m_origEdgeFuncp);

    // Make a new graph with vertices that have only a single vertex
    // for each group of old vertices that are interconnected with unbreakable
    // edges (and thus can't represent loops - if we did the unbreakable
    // marking right, anyways)
    buildGraph(m_origGraphp);
    ಯದಿ (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_pre");

    // Perform simple optimizations before any cuttings
    simplify(ಅಸತ್ಯ);
    ಯದಿ (dumpGraphLevel() >= 5) m_breakGraph.dumpDotFilePrefixed("acyc_simp");

    UINFO(4, " Cutting trivial loops");
    simplify(ಸತ್ಯ);
    ಯದಿ (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_mid");

    UINFO(4, " Ranking");
    m_breakGraph.rank(&V3GraphEdge::followNotCutable);
    ಯದಿ (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_rank");

    UINFO(4, " Placement");
    place();
    ಯದಿ (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_place");

    UINFO(4, " Final Ranking");
    // Only needed to assert there are no loops in completed graph
    m_breakGraph.rank(&V3GraphEdge::followAlwaysTrue);
    ಯದಿ (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_done");
}

ಶೂನ್ಯ V3Graph::acyclic(V3EdgeFuncP edgeFuncp) {
    UINFO(4, "Acyclic");
    GraphAcyc acyc{ಏತತ್, edgeFuncp};
    acyc.main();
    UINFO(4, "Acyclic done");
}
