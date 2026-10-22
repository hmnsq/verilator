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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Global.h"
#অন্তর্ভাৱ "V3Graph.h"

#অন্তর্ভাৱ <algorithm>
#অন্তর্ভাৱ <list>
#অন্তর্ভাৱ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - acyclic
//      Break the minimal number of backward edges to make the graph acyclic

শ্ৰেণী GraphAcycVertex final : সার্ৱজনিক V3GraphVertex {
    VL_RTTI_IMPL(GraphAcycVertex, V3GraphVertex)
    // user() is used for various sub-algorithm pieces
    V3GraphVertex* ধ্রুৱ m_origVertexp;  // Pointer to first vertex this represents
রক্ষিত:
    friend শ্ৰেণী GraphAcyc;
    V3ListLinks<GraphAcycVertex> m_links;  // List links to store instances of this class
    uint32_t m_storedRank = 0;  // Rank held until commit to edge placement
    তার্কিক m_onWorkList = মিছা;  // True if already on list of work to do
    তার্কিক m_deleted = মিছা;  // True if deleted

গুপ্ত:
    V3ListLinks<GraphAcycVertex>& links() { ঘূৰাই m_links; }

সার্ৱজনিক:
    // List type to store instances of this class
    using List = V3List<GraphAcycVertex, &GraphAcycVertex::links>;

    GraphAcycVertex(V3Graph* graphp, V3GraphVertex* origVertexp)
        : V3GraphVertex{graphp}
        , m_origVertexp{origVertexp} {}
    ~GraphAcycVertex() override = স্ৱভাৱ;
    V3GraphVertex* origVertexp() ধ্রুৱ { ঘূৰাই m_origVertexp; }
    শূন্য setDelete() { m_deleted = সঁচা; }
    তার্কিক isDelete() ধ্রুৱ { ঘূৰাই m_deleted; }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string name() ধ্রুৱ override { ঘূৰাই m_origVertexp->name(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string dotColor() ধ্রুৱ override { ঘূৰাই m_origVertexp->dotColor(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    File_সাৰি* fileline() ধ্রুৱ override { ঘূৰাই m_origVertexp->fileline(); }
};

//--------------------------------------------------------------------

শ্ৰেণী GraphAcycEdge final : সার্ৱজনিক V3GraphEdge {
    VL_RTTI_IMPL(GraphAcycEdge, V3GraphEdge)
    // userp() is always used to point to the head original graph edge
গুপ্ত:
    using OrigEdgeList = std::list<V3GraphEdge*>;  // List of orig edges, see also GraphAcyc's decl
    V3GraphEdge* origEdgep() ধ্রুৱ {
        ধ্রুৱ OrigEdgeList* ধ্রুৱ oEListp = static_cast<OrigEdgeList*>(userp());
        UASSERT(oEListp, "No original edge associated with acyc edge " << এতত্);
        ঘূৰাই (oEListp->front());
    }

সার্ৱজনিক:
    GraphAcycEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীৰ্ষ, পূর্ণাঙ্ক weight,
                  তার্কিক cutable = মিছা)
        : V3GraphEdge{graphp, fromp, শীৰ্ষ, weight, cutable} {}
    ~GraphAcycEdge() override = স্ৱভাৱ;
    // yellow=we might still cut it, else oldEdge: yellowGreen=made uncutable, red=uncutable
    string dotColor() ধ্রুৱ override { ঘূৰাই (cutable() ? "yellow" : origEdgep()->dotColor()); }
};

//--------------------------------------------------------------------

সংরচনা GraphAcycEdgeCmp final {
    তার্কিক operator()(ধ্রুৱ V3GraphEdge* lhsp, ধ্রুৱ V3GraphEdge* rhsp) ধ্রুৱ {
        যদি (lhsp->weight() > rhsp->weight()) ঘূৰাই সঁচা;  // LHS goes first
        যদি (lhsp->weight() < rhsp->weight()) ঘূৰাই মিছা;  // RHS goes first
        ঘূৰাই মিছা;
    }
};

//--------------------------------------------------------------------

// CLASSES
শ্ৰেণী GraphAcyc final {
    using OrigEdgeList
        = std::list<V3GraphEdge*>;  // List of orig edges, see also GraphAcycEdge's decl
    // GRAPH USERS
    //  origGraph
    //    GraphVertex::user()   GraphAycVerted* New graph node
    //  m_breakGraph
    //    GraphEdge::user()     OrigEdgeList*   Old graph edges
    //    GraphVertex::user     bool            Detection of loops in simplifyDupIterate
    // MEMBERS
    V3Graph* ধ্রুৱ m_origGraphp;  // Original graph
    V3Graph m_breakGraph;  // Graph with only breakable edges represented
    GraphAcycVertex::List m_work;  // List of vertices with optimization work left
    std::vector<OrigEdgeList*> m_origEdgeDelp;  // List of deletions to do when done
    ধ্রুৱ V3EdgeFuncP
        m_origEdgeFuncp;  // Function that says we follow this edge (in original graph)
    uint32_t m_placeStep = 0;  // Number that user() must be equal to to indicate processing

    // METHODS
    শূন্য buildGraph(V3Graph* origGraphp);
    শূন্য buildGraphIterate(V3GraphVertex* overtexp, GraphAcycVertex* avertexp);
    শূন্য simplify(তার্কিক allowCut);
    শূন্য simplifyNone(GraphAcycVertex* avertexp);
    শূন্য simplifyOne(GraphAcycVertex* avertexp);
    শূন্য simplifyOut(GraphAcycVertex* avertexp);
    শূন্য simplifyDup(GraphAcycVertex* avertexp);
    শূন্য cutBasic(GraphAcycVertex* avertexp);
    শূন্য cutBackward(GraphAcycVertex* avertexp);
    শূন্য deleteMarked();
    শূন্য place();
    শূন্য placeTryEdge(V3GraphEdge* edgep);
    তার্কিক placeIterate(GraphAcycVertex* vertexp, uint32_t currentRank);

    তার্কিক origFollowEdge(V3GraphEdge* edgep) {
        ঘূৰাই (edgep->weight() && (m_origEdgeFuncp)(edgep));
    }
    শূন্য edgeFromEdge(V3GraphEdge* oldedgep, V3GraphVertex* fromp, V3GraphVertex* শীৰ্ষ) {
        // Make new breakGraph edge, with old edge as a template
        GraphAcycEdge* ধ্রুৱ newEdgep = নূতন GraphAcycEdge{&m_breakGraph, fromp, শীৰ্ষ,
                                                          oldedgep->weight(), oldedgep->cutable()};
        newEdgep->userp(oldedgep->userp());  // Keep pointer to OrigEdgeList
    }
    শূন্য addOrigEdgep(V3GraphEdge* toEdgep, V3GraphEdge* addEdgep) {
        // Add addEdge (or it's list) to list of edges that break edge represents
        // Note addEdge may already have a bunch of similar linked edge representations.  Yuk.
        UASSERT(addEdgep, "Adding nullptr");
        যদি (!toEdgep->userp()) {
            OrigEdgeList* ধ্রুৱ oep = নূতন OrigEdgeList;
            m_origEdgeDelp.ঠেলো_back(oep);
            toEdgep->userp(oep);
        }
        OrigEdgeList* ধ্রুৱ oEListp = static_cast<OrigEdgeList*>(toEdgep->userp());
        যদি (OrigEdgeList* ধ্রুৱ addListp = static_cast<OrigEdgeList*>(addEdgep->userp())) {
            বাবে (ধ্রুৱ auto& itr : *addListp) oEListp->ঠেলো_back(itr);
            addListp->clear();  // Done with it
        } অন্যথা {
            oEListp->ঠেলো_back(addEdgep);
        }
    }
    শূন্য cutOrigEdge(V3GraphEdge* breakEdgep, ধ্রুৱ ৱর্ণ* why) {
        // From the break edge, cut edges in original graph it represents
        UINFO(8, why << " CUT " << breakEdgep->fromp());
        breakEdgep->cut();
        ধ্রুৱ OrigEdgeList* ধ্রুৱ oEListp = static_cast<OrigEdgeList*>(breakEdgep->userp());
        যদি (!oEListp) {
            v3fatalSrc("No original edge associated with cutting edge " << breakEdgep);
        }
        // The breakGraph edge may represent multiple real edges; cut them all
        বাবে (ধ্রুৱ auto& origEdgep : *oEListp) {
            origEdgep->cut();
            UINFO(8, "  " << why << "   " << origEdgep->fromp() << " ->" << origEdgep->শীৰ্ষ());
        }
    }
    // Work Queue
    শূন্য work_ঠেলো(V3GraphVertex* vertexp) {
        GraphAcycVertex* ধ্রুৱ avertexp = static_cast<GraphAcycVertex*>(vertexp);
        // Add vertex to list of nodes needing further optimization trials
        যদি (!avertexp->m_onWorkList) {
            avertexp->m_onWorkList = সঁচা;
            m_work.linkBack(avertexp);
        }
    }
    GraphAcycVertex* workBeginp() { ঘূৰাই m_work.frontp(); }
    শূন্য work_বেৰ_কৰো() {
        GraphAcycVertex* ধ্রুৱ avertexp = workBeginp();
        avertexp->m_onWorkList = মিছা;
        m_work.unlink(avertexp);
    }

সার্ৱজনিক:
    // CONSTRUCTORS
    GraphAcyc(V3Graph* origGraphp, V3EdgeFuncP edgeFuncp)
        : m_origGraphp{origGraphp}
        , m_origEdgeFuncp{edgeFuncp} {}
    ~GraphAcyc() {
        বাবে (OrigEdgeList* ip : m_origEdgeDelp) delete ip;
        m_origEdgeDelp.clear();
    }
    শূন্য main();
};

//--------------------------------------------------------------------

শূন্য GraphAcyc::buildGraph(V3Graph* origGraphp) {
    // Presumes the graph has been strongly ordered,
    // and thus there's a unique color if there are loops in this subgraph.

    // For each old node, make a new graph node for optimization
    origGraphp->userClearVertices();
    origGraphp->userClearEdges();
    বাবে (V3GraphVertex& overtex : origGraphp->vertices()) {
        যদি (overtex.color()) {
            GraphAcycVertex* ধ্রুৱ avertexp = নূতন GraphAcycVertex{&m_breakGraph, &overtex};
            overtex.userp(avertexp);  // Stash so can look up later
        }
    }

    // Build edges between logic vertices
    বাবে (V3GraphVertex& overtex : origGraphp->vertices()) {
        যদি (overtex.color()) {
            GraphAcycVertex* ধ্রুৱ avertexp = static_cast<GraphAcycVertex*>(overtex.userp());
            buildGraphIterate(&overtex, avertexp);
        }
    }
}

শূন্য GraphAcyc::buildGraphIterate(V3GraphVertex* overtexp, GraphAcycVertex* avertexp) {
    // Make new edges
    বাবে (V3GraphEdge& edge : overtexp->outEdges()) {
        যদি (origFollowEdge(&edge)) {  // not cut
            ধ্রুৱ V3GraphVertex* toVertexp = edge.শীৰ্ষ();
            যদি (toVertexp->color()) {
                GraphAcycVertex* ধ্রুৱ toAVertexp
                    = static_cast<GraphAcycVertex*>(toVertexp->userp());
                // Replicate the old edge into the new graph
                // There may be multiple edges between same pairs of vertices
                V3GraphEdge* breakEdgep = নূতন GraphAcycEdge{&m_breakGraph, avertexp, toAVertexp,
                                                            edge.weight(), edge.cutable()};
                addOrigEdgep(breakEdgep, &edge);  // So can find original edge
            }
        }
    }
}

শূন্য GraphAcyc::simplify(তার্কিক allowCut) {
    // Add all nodes to list of work to do
    বাবে (V3GraphVertex& vertex : m_breakGraph.vertices()) work_ঠেলো(&vertex);
    // Optimize till everything finished
    যেতিয়ালৈ (GraphAcycVertex* vertexp = workBeginp()) {
        work_বেৰ_কৰো();
        simplifyNone(vertexp);
        simplifyOne(vertexp);
        simplifyOut(vertexp);
        simplifyDup(vertexp);
        যদি (allowCut) {
            // The main algorithm works without these, though slower
            // So if changing the main algorithm, comment these out for a test run
            যদি (v3Global.opt.fAcycSimp()) {
                cutBasic(vertexp);
                cutBackward(vertexp);
            }
        }
    }
    deleteMarked();
}

শূন্য GraphAcyc::deleteMarked() {
    // Delete nodes marked for removal
    বাবে (V3GraphVertex* ধ্রুৱ vtxp : m_breakGraph.vertices().unlinkable()) {
        GraphAcycVertex* ধ্রুৱ avertexp = static_cast<GraphAcycVertex*>(vtxp);
        যদি (avertexp->isDelete()) {
            VL_DO_DANGLING(avertexp->unlinkDelete(&m_breakGraph), avertexp);
        }
    }
}

শূন্য GraphAcyc::simplifyNone(GraphAcycVertex* avertexp) {
    // Don't need any vertices with no inputs, There's no way they can have a loop.
    // Likewise, vertices with no outputs
    যদি (avertexp->isDelete()) ঘূৰাই;
    যদি (avertexp->inEmpty() || avertexp->outEmpty()) {
        UINFO(9, "  SimplifyNoneRemove " << avertexp);
        avertexp->setDelete();  // Mark so we won't delete it twice
        // Remove edges
        যেতিয়ালৈ (V3GraphEdge* ধ্রুৱ edgep = avertexp->outEdges().frontp()) {
            work_ঠেলো(edgep->শীৰ্ষ());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        যেতিয়ালৈ (V3GraphEdge* ধ্রুৱ edgep = avertexp->inEdges().frontp()) {
            work_ঠেলো(edgep->fromp());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
}

শূন্য GraphAcyc::simplifyOne(GraphAcycVertex* avertexp) {
    // If a node has one input and one output, we can remove it and change the edges
    যদি (avertexp->isDelete()) ঘূৰাই;
    যদি (avertexp->inSize1() && avertexp->outSize1()) {
        V3GraphEdge* ধ্রুৱ inEdgep = avertexp->inEdges().frontp();
        V3GraphEdge* ধ্রুৱ outEdgep = avertexp->outEdges().frontp();
        V3GraphVertex* ধ্রুৱ inVertexp = inEdgep->fromp();
        V3GraphVertex* ধ্রুৱ outVertexp = outEdgep->শীৰ্ষ();
        // The in and out may be the same node; we'll make a loop
        // The in OR out may be THIS node; we can't delete it then.
        যদি (inVertexp != avertexp && outVertexp != avertexp) {
            UINFO(9, "  SimplifyOneRemove " << avertexp);
            avertexp->setDelete();  // Mark so we won't delete it twice
            // Make a new edge connecting the two vertices directly
            // If both are breakable, we pick the one with less weight, else it's arbitrary
            // We can forget about the origEdge list for the "non-selected" set of edges,
            // as we need to break only one set or the other set of edges, not both.
            // (This is why we must give preference to the cutable set.)
            V3GraphEdge* ধ্রুৱ templateEdgep
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
            work_ঠেলো(inVertexp);
            work_ঠেলো(outVertexp);
        }
    }
}

শূন্য GraphAcyc::simplifyOut(GraphAcycVertex* avertexp) {
    // If a node has one output that's not cutable, all its inputs can be reassigned
    // to the next node in the list
    যদি (avertexp->isDelete()) ঘূৰাই;
    যদি (avertexp->outSize1()) {
        V3GraphEdge* ধ্রুৱ outEdgep = avertexp->outEdges().frontp();
        যদি (!outEdgep->cutable()) {
            V3GraphVertex* outVertexp = outEdgep->শীৰ্ষ();
            UINFO(9, "  SimplifyOutRemove " << avertexp);
            avertexp->setDelete();  // Mark so we won't delete it twice
            বাবে (V3GraphEdge* ধ্রুৱ inEdgep : avertexp->inEdges().unlinkable()) {
                V3GraphVertex* inVertexp = inEdgep->fromp();
                যদি (inVertexp == avertexp) {
                    যদি (debug()) v3error("Non-cutable vertex=" << avertexp);  // LCOV_EXCL_LINE
                    v3error("Circular logic when ordering code (non-cutable edge loop)\n"
                            << m_origGraphp->reportLoops(  // calls OrderGraph::loopsVertexCb
                                   &V3GraphEdge::followNotCutable, avertexp->origVertexp()));
                    // Things are unlikely to end well at this point,
                    // but we'll try something to get to further errors...
                    inEdgep->cutable(সঁচা);
                    ঘূৰাই;
                }
                // Make a new edge connecting the two vertices directly
                // cppcheck-suppress leakReturnValNotUsed
                edgeFromEdge(inEdgep, inVertexp, outVertexp);
                // Remove old edge
                VL_DO_DANGLING(inEdgep->unlinkDelete(), inEdgep);
                work_ঠেলো(inVertexp);
            }
            VL_DO_DANGLING(outEdgep->unlinkDelete(), outEdgep);
            work_ঠেলো(outVertexp);
        }
    }
}

শূন্য GraphAcyc::simplifyDup(GraphAcycVertex* avertexp) {
    // Remove redundant edges
    যদি (avertexp->isDelete()) ঘূৰাই;
    // Clear marks
    বাবে (V3GraphEdge& edge : avertexp->outEdges()) edge.শীৰ্ষ()->userp(nullptr);
    // Mark edges and detect duplications
    বাবে (V3GraphEdge* ধ্রুৱ edgep : avertexp->outEdges().unlinkable()) {
        V3GraphVertex* outVertexp = edgep->শীৰ্ষ();
        V3GraphEdge* prevEdgep = static_cast<V3GraphEdge*>(outVertexp->userp());
        যদি (prevEdgep) {
            যদি (!prevEdgep->cutable()) {
                // !cutable duplicates prev !cutable: we can ignore it, redundant
                //  cutable duplicates prev !cutable: know it's not a relevant loop, ignore it
                UINFO(8, "    DelDupEdge " << avertexp << " -> " << edgep->শীৰ্ষ());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            } অন্যথা যদি (!edgep->cutable()) {
                // !cutable duplicates prev  cutable: delete the earlier cutable
                UINFO(8, "    DelDupPrev " << avertexp << " -> " << prevEdgep->শীৰ্ষ());
                VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                outVertexp->userp(edgep);
            } অন্যথা {
                //  cutable duplicates prev  cutable: combine weights
                UINFO(8, "    DelDupComb " << avertexp << " -> " << edgep->শীৰ্ষ());
                prevEdgep->weight(prevEdgep->weight() + edgep->weight());
                addOrigEdgep(prevEdgep, edgep);
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            work_ঠেলো(outVertexp);
            work_ঠেলো(avertexp);
        } অন্যথা {
            // No previous assignment
            outVertexp->userp(edgep);
        }
    }
}

শূন্য GraphAcyc::cutBasic(GraphAcycVertex* avertexp) {
    // Detect and cleanup any loops from node to itself
    যদি (avertexp->isDelete()) ঘূৰাই;
    বাবে (V3GraphEdge* ধ্রুৱ edgep : avertexp->outEdges().unlinkable()) {
        যদি (edgep->cutable() && edgep->শীৰ্ষ() == avertexp) {
            cutOrigEdge(edgep, "  Cut Basic");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_ঠেলো(avertexp);
        }
    }
}

শূন্য GraphAcyc::cutBackward(GraphAcycVertex* avertexp) {
    // If a cutable edge is from A->B, and there's a non-cutable edge B->A, then must cut!
    যদি (avertexp->isDelete()) ঘূৰাই;
    // Clear marks
    বাবে (V3GraphEdge& edge : avertexp->outEdges()) edge.শীৰ্ষ()->user(মিছা);
    বাবে (V3GraphEdge& edge : avertexp->inEdges()) {
        যদি (!edge.cutable()) edge.fromp()->user(সঁচা);
    }
    // Detect duplications
    বাবে (V3GraphEdge* ধ্রুৱ edgep : avertexp->outEdges().unlinkable()) {
        যদি (edgep->cutable() && edgep->শীৰ্ষ()->user()) {
            cutOrigEdge(edgep, "  Cut A->B->A");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_ঠেলো(avertexp);
        }
    }
}

শূন্য GraphAcyc::place() {
    // Input is m_breakGraph with ranks already assigned on non-breakable edges

    // Make a list of all cutable edges in the graph
    পূর্ণাঙ্ক অঙ্ক_Edges = 0;
    বাবে (V3GraphVertex& vertex : m_breakGraph.vertices()) {
        বাবে (V3GraphEdge& edge : vertex.outEdges()) {
            যদি (edge.weight() && edge.cutable()) ++অঙ্ক_Edges;
        }
    }
    UINFO(4, "    Cutable edges = " << অঙ্ক_Edges);

    std::vector<V3GraphEdge*> edges;  // List of all edges to be processed
    // Make the vector properly sized right off the bat -- faster than reallocating
    edges.reserve(অঙ্ক_Edges + 1);
    বাবে (V3GraphVertex& vertex : m_breakGraph.vertices()) {
        vertex.user(0);  // Clear in prep of next step
        বাবে (V3GraphEdge& edge : vertex.outEdges()) {
            যদি (edge.weight() && edge.cutable()) edges.ঠেলো_back(&edge);
        }
    }

    // Sort by weight, then by vertex (so that we completely process one vertex, when possible)
    stable_sort(edges.begin(), edges.end(), GraphAcycEdgeCmp());

    // Process each edge in weighted order
    m_placeStep = 10;
    বাবে (V3GraphEdge* edgep : edges) placeTryEdge(edgep);
}

শূন্য GraphAcyc::placeTryEdge(V3GraphEdge* edgep) {
    // Try to make this edge uncutable
    m_placeStep++;
    UINFO(8, "    PlaceEdge s" << m_placeStep << " w" << edgep->weight() << " " << edgep->fromp());
    // Make the edge uncutable so we detect it in placement
    edgep->cutable(মিছা);
    // Vertex::m_user begin: number indicates this edge was completed
    // Try to assign ranks, presuming this edge is in place
    // If we come across user()==placestep, we've detected a loop and must back out
    ধ্রুৱ তার্কিক loop
        = placeIterate(static_cast<GraphAcycVertex*>(edgep->শীৰ্ষ()), edgep->fromp()->rank() + 1);
    যদি (!loop) {
        // No loop, we can keep it as uncutable
        // Commit the new ranks we calculated
        // Just cleanup the list.  If this is slow, we can add another set of
        // user counters to avoid cleaning up the list.
        যেতিয়ালৈ (workBeginp()) work_বেৰ_কৰো();
    } অন্যথা {
        // Adding this edge would cause a loop, kill it
        edgep->cutable(সঁচা);  // So graph still looks pretty
        cutOrigEdge(edgep, "  Cut loop");
        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        // Back out the ranks we calculated
        যেতিয়ালৈ (GraphAcycVertex* vertexp = workBeginp()) {
            work_বেৰ_কৰো();
            vertexp->rank(vertexp->m_storedRank);
        }
    }
}

তার্কিক GraphAcyc::placeIterate(GraphAcycVertex* vertexp, uint32_t currentRank) {
    // Assign rank to each unvisited node
    //   rank() is the "committed rank" of the graph known without loops
    // If larger rank is found, assign it and loop back through
    // If we hit a back node make a list of all loops
    যদি (vertexp->rank() >= currentRank) ঘূৰাই মিছা;  // Already processed it
    যদি (vertexp->user() == m_placeStep) ঘূৰাই সঁচা;  // Loop detected
    vertexp->user(m_placeStep);
    // Remember we're changing the rank of this node; might need to back out
    যদি (!vertexp->m_onWorkList) {
        vertexp->m_storedRank = vertexp->rank();
        work_ঠেলো(vertexp);
    }
    vertexp->rank(currentRank);
    // Follow all edges and increase their ranks
    বাবে (V3GraphEdge& edge : vertexp->outEdges()) {
        যদি (edge.weight() && !edge.cutable()) {
            যদি (placeIterate(static_cast<GraphAcycVertex*>(edge.শীৰ্ষ()), currentRank + 1)) {
                // We don't need to reset user(); we'll use a different placeStep for the next edge
                ঘূৰাই সঁচা;  // Loop detected
            }
        }
    }
    vertexp->user(0);
    ঘূৰাই মিছা;
}

//----- Main algorithm entry point

শূন্য GraphAcyc::main() {
    m_breakGraph.userClearEdges();

    // Color based on possible loops
    m_origGraphp->stronglyConnected(m_origEdgeFuncp);

    // Make a new graph with vertices that have only a single vertex
    // for each group of old vertices that are interconnected with unbreakable
    // edges (and thus can't represent loops - if we did the unbreakable
    // marking right, anyways)
    buildGraph(m_origGraphp);
    যদি (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_pre");

    // Perform simple optimizations before any cuttings
    simplify(মিছা);
    যদি (dumpGraphLevel() >= 5) m_breakGraph.dumpDotFilePrefixed("acyc_simp");

    UINFO(4, " Cutting trivial loops");
    simplify(সঁচা);
    যদি (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_mid");

    UINFO(4, " Ranking");
    m_breakGraph.rank(&V3GraphEdge::followNotCutable);
    যদি (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_rank");

    UINFO(4, " Placement");
    place();
    যদি (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_place");

    UINFO(4, " Final Ranking");
    // Only needed to assert there are no loops in completed graph
    m_breakGraph.rank(&V3GraphEdge::followAlwaysTrue);
    যদি (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_done");
}

শূন্য V3Graph::acyclic(V3EdgeFuncP edgeFuncp) {
    UINFO(4, "Acyclic");
    GraphAcyc acyc{এতত্, edgeFuncp};
    acyc.main();
    UINFO(4, "Acyclic done");
}
