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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Graph.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <list>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - acyclic
//      Break the minimal number of backward edges to make the graph acyclic

वर्ग GraphAcycVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(GraphAcycVertex, V3GraphVertex)
    // user() is used for various sub-algorithm pieces
    V3GraphVertex* ध्रुव m_origVertexp;  // Pointer to first vertex this represents
रक्षित:
    friend वर्ग GraphAcyc;
    V3ListLinks<GraphAcycVertex> m_links;  // List links to store instances of this class
    uint32_t m_storedRank = 0;  // Rank held until commit to edge placement
    तार्किक m_onWorkList = असत्य;  // True if already on list of work to do
    तार्किक m_deleted = असत्य;  // True if deleted

गुप्त:
    V3ListLinks<GraphAcycVertex>& links() { प्रत्यागम m_links; }

सार्वजनिक:
    // List type to store instances of this class
    using List = V3List<GraphAcycVertex, &GraphAcycVertex::links>;

    GraphAcycVertex(V3Graph* graphp, V3GraphVertex* origVertexp)
        : V3GraphVertex{graphp}
        , m_origVertexp{origVertexp} {}
    ~GraphAcycVertex() override = स्वभाव;
    V3GraphVertex* origVertexp() ध्रुव { प्रत्यागम m_origVertexp; }
    शून्य setDelete() { m_deleted = सत्य; }
    तार्किक isDelete() ध्रुव { प्रत्यागम m_deleted; }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string name() ध्रुव override { प्रत्यागम m_origVertexp->name(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string dotColor() ध्रुव override { प्रत्यागम m_origVertexp->dotColor(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    File_ओळ* fileline() ध्रुव override { प्रत्यागम m_origVertexp->fileline(); }
};

//--------------------------------------------------------------------

वर्ग GraphAcycEdge final : सार्वजनिक V3GraphEdge {
    VL_RTTI_IMPL(GraphAcycEdge, V3GraphEdge)
    // userp() is always used to point to the head original graph edge
गुप्त:
    using OrigEdgeList = std::list<V3GraphEdge*>;  // List of orig edges, see also GraphAcyc's decl
    V3GraphEdge* origEdgep() ध्रुव {
        ध्रुव OrigEdgeList* ध्रुव oEListp = static_cast<OrigEdgeList*>(userp());
        UASSERT(oEListp, "No original edge associated with acyc edge " << एतत्);
        प्रत्यागम (oEListp->front());
    }

सार्वजनिक:
    GraphAcycEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णाङ्क weight,
                  तार्किक cutable = असत्य)
        : V3GraphEdge{graphp, fromp, शीर्ष, weight, cutable} {}
    ~GraphAcycEdge() override = स्वभाव;
    // yellow=we might still cut it, else oldEdge: yellowGreen=made uncutable, red=uncutable
    string dotColor() ध्रुव override { प्रत्यागम (cutable() ? "yellow" : origEdgep()->dotColor()); }
};

//--------------------------------------------------------------------

संरचना GraphAcycEdgeCmp final {
    तार्किक operator()(ध्रुव V3GraphEdge* lhsp, ध्रुव V3GraphEdge* rhsp) ध्रुव {
        यदि (lhsp->weight() > rhsp->weight()) प्रत्यागम सत्य;  // LHS goes first
        यदि (lhsp->weight() < rhsp->weight()) प्रत्यागम असत्य;  // RHS goes first
        प्रत्यागम असत्य;
    }
};

//--------------------------------------------------------------------

// CLASSES
वर्ग GraphAcyc final {
    using OrigEdgeList
        = std::list<V3GraphEdge*>;  // List of orig edges, see also GraphAcycEdge's decl
    // GRAPH USERS
    //  origGraph
    //    GraphVertex::user()   GraphAycVerted* New graph node
    //  m_breakGraph
    //    GraphEdge::user()     OrigEdgeList*   Old graph edges
    //    GraphVertex::user     bool            Detection of loops in simplifyDupIterate
    // MEMBERS
    V3Graph* ध्रुव m_origGraphp;  // Original graph
    V3Graph m_breakGraph;  // Graph with only breakable edges represented
    GraphAcycVertex::List m_work;  // List of vertices with optimization work left
    std::vector<OrigEdgeList*> m_origEdgeDelp;  // List of deletions to do when done
    ध्रुव V3EdgeFuncP
        m_origEdgeFuncp;  // Function that says we follow this edge (in original graph)
    uint32_t m_placeStep = 0;  // Number that user() must be equal to to indicate processing

    // METHODS
    शून्य buildGraph(V3Graph* origGraphp);
    शून्य buildGraphIterate(V3GraphVertex* overtexp, GraphAcycVertex* avertexp);
    शून्य simplify(तार्किक allowCut);
    शून्य simplifyNone(GraphAcycVertex* avertexp);
    शून्य simplifyOne(GraphAcycVertex* avertexp);
    शून्य simplifyOut(GraphAcycVertex* avertexp);
    शून्य simplifyDup(GraphAcycVertex* avertexp);
    शून्य cutBasic(GraphAcycVertex* avertexp);
    शून्य cutBackward(GraphAcycVertex* avertexp);
    शून्य deleteMarked();
    शून्य place();
    शून्य placeTryEdge(V3GraphEdge* edgep);
    तार्किक placeIterate(GraphAcycVertex* vertexp, uint32_t currentRank);

    तार्किक origFollowEdge(V3GraphEdge* edgep) {
        प्रत्यागम (edgep->weight() && (m_origEdgeFuncp)(edgep));
    }
    शून्य edgeFromEdge(V3GraphEdge* oldedgep, V3GraphVertex* fromp, V3GraphVertex* शीर्ष) {
        // Make new breakGraph edge, with old edge as a template
        GraphAcycEdge* ध्रुव newEdgep = नूतन GraphAcycEdge{&m_breakGraph, fromp, शीर्ष,
                                                          oldedgep->weight(), oldedgep->cutable()};
        newEdgep->userp(oldedgep->userp());  // Keep pointer to OrigEdgeList
    }
    शून्य addOrigEdgep(V3GraphEdge* toEdgep, V3GraphEdge* addEdgep) {
        // Add addEdge (or it's list) to list of edges that break edge represents
        // Note addEdge may already have a bunch of similar linked edge representations.  Yuk.
        UASSERT(addEdgep, "Adding nullptr");
        यदि (!toEdgep->userp()) {
            OrigEdgeList* ध्रुव oep = नूतन OrigEdgeList;
            m_origEdgeDelp.ढकला_back(oep);
            toEdgep->userp(oep);
        }
        OrigEdgeList* ध्रुव oEListp = static_cast<OrigEdgeList*>(toEdgep->userp());
        यदि (OrigEdgeList* ध्रुव addListp = static_cast<OrigEdgeList*>(addEdgep->userp())) {
            प्रति (ध्रुव auto& itr : *addListp) oEListp->ढकला_back(itr);
            addListp->clear();  // Done with it
        } अन्यथा {
            oEListp->ढकला_back(addEdgep);
        }
    }
    शून्य cutOrigEdge(V3GraphEdge* breakEdgep, ध्रुव वर्ण* why) {
        // From the break edge, cut edges in original graph it represents
        UINFO(8, why << " CUT " << breakEdgep->fromp());
        breakEdgep->cut();
        ध्रुव OrigEdgeList* ध्रुव oEListp = static_cast<OrigEdgeList*>(breakEdgep->userp());
        यदि (!oEListp) {
            v3fatalSrc("No original edge associated with cutting edge " << breakEdgep);
        }
        // The breakGraph edge may represent multiple real edges; cut them all
        प्रति (ध्रुव auto& origEdgep : *oEListp) {
            origEdgep->cut();
            UINFO(8, "  " << why << "   " << origEdgep->fromp() << " ->" << origEdgep->शीर्ष());
        }
    }
    // Work Queue
    शून्य work_ढकला(V3GraphVertex* vertexp) {
        GraphAcycVertex* ध्रुव avertexp = static_cast<GraphAcycVertex*>(vertexp);
        // Add vertex to list of nodes needing further optimization trials
        यदि (!avertexp->m_onWorkList) {
            avertexp->m_onWorkList = सत्य;
            m_work.linkBack(avertexp);
        }
    }
    GraphAcycVertex* workBeginp() { प्रत्यागम m_work.frontp(); }
    शून्य work_काढा() {
        GraphAcycVertex* ध्रुव avertexp = workBeginp();
        avertexp->m_onWorkList = असत्य;
        m_work.unlink(avertexp);
    }

सार्वजनिक:
    // CONSTRUCTORS
    GraphAcyc(V3Graph* origGraphp, V3EdgeFuncP edgeFuncp)
        : m_origGraphp{origGraphp}
        , m_origEdgeFuncp{edgeFuncp} {}
    ~GraphAcyc() {
        प्रति (OrigEdgeList* ip : m_origEdgeDelp) delete ip;
        m_origEdgeDelp.clear();
    }
    शून्य main();
};

//--------------------------------------------------------------------

शून्य GraphAcyc::buildGraph(V3Graph* origGraphp) {
    // Presumes the graph has been strongly ordered,
    // and thus there's a unique color if there are loops in this subgraph.

    // For each old node, make a new graph node for optimization
    origGraphp->userClearVertices();
    origGraphp->userClearEdges();
    प्रति (V3GraphVertex& overtex : origGraphp->vertices()) {
        यदि (overtex.color()) {
            GraphAcycVertex* ध्रुव avertexp = नूतन GraphAcycVertex{&m_breakGraph, &overtex};
            overtex.userp(avertexp);  // Stash so can look up later
        }
    }

    // Build edges between logic vertices
    प्रति (V3GraphVertex& overtex : origGraphp->vertices()) {
        यदि (overtex.color()) {
            GraphAcycVertex* ध्रुव avertexp = static_cast<GraphAcycVertex*>(overtex.userp());
            buildGraphIterate(&overtex, avertexp);
        }
    }
}

शून्य GraphAcyc::buildGraphIterate(V3GraphVertex* overtexp, GraphAcycVertex* avertexp) {
    // Make new edges
    प्रति (V3GraphEdge& edge : overtexp->outEdges()) {
        यदि (origFollowEdge(&edge)) {  // not cut
            ध्रुव V3GraphVertex* toVertexp = edge.शीर्ष();
            यदि (toVertexp->color()) {
                GraphAcycVertex* ध्रुव toAVertexp
                    = static_cast<GraphAcycVertex*>(toVertexp->userp());
                // Replicate the old edge into the new graph
                // There may be multiple edges between same pairs of vertices
                V3GraphEdge* breakEdgep = नूतन GraphAcycEdge{&m_breakGraph, avertexp, toAVertexp,
                                                            edge.weight(), edge.cutable()};
                addOrigEdgep(breakEdgep, &edge);  // So can find original edge
            }
        }
    }
}

शून्य GraphAcyc::simplify(तार्किक allowCut) {
    // Add all nodes to list of work to do
    प्रति (V3GraphVertex& vertex : m_breakGraph.vertices()) work_ढकला(&vertex);
    // Optimize till everything finished
    यावत् (GraphAcycVertex* vertexp = workBeginp()) {
        work_काढा();
        simplifyNone(vertexp);
        simplifyOne(vertexp);
        simplifyOut(vertexp);
        simplifyDup(vertexp);
        यदि (allowCut) {
            // The main algorithm works without these, though slower
            // So if changing the main algorithm, comment these out for a test run
            यदि (v3Global.opt.fAcycSimp()) {
                cutBasic(vertexp);
                cutBackward(vertexp);
            }
        }
    }
    deleteMarked();
}

शून्य GraphAcyc::deleteMarked() {
    // Delete nodes marked for removal
    प्रति (V3GraphVertex* ध्रुव vtxp : m_breakGraph.vertices().unlinkable()) {
        GraphAcycVertex* ध्रुव avertexp = static_cast<GraphAcycVertex*>(vtxp);
        यदि (avertexp->isDelete()) {
            VL_DO_DANGLING(avertexp->unlinkDelete(&m_breakGraph), avertexp);
        }
    }
}

शून्य GraphAcyc::simplifyNone(GraphAcycVertex* avertexp) {
    // Don't need any vertices with no inputs, There's no way they can have a loop.
    // Likewise, vertices with no outputs
    यदि (avertexp->isDelete()) प्रत्यागम;
    यदि (avertexp->inEmpty() || avertexp->outEmpty()) {
        UINFO(9, "  SimplifyNoneRemove " << avertexp);
        avertexp->setDelete();  // Mark so we won't delete it twice
        // Remove edges
        यावत् (V3GraphEdge* ध्रुव edgep = avertexp->outEdges().frontp()) {
            work_ढकला(edgep->शीर्ष());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        यावत् (V3GraphEdge* ध्रुव edgep = avertexp->inEdges().frontp()) {
            work_ढकला(edgep->fromp());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
}

शून्य GraphAcyc::simplifyOne(GraphAcycVertex* avertexp) {
    // If a node has one input and one output, we can remove it and change the edges
    यदि (avertexp->isDelete()) प्रत्यागम;
    यदि (avertexp->inSize1() && avertexp->outSize1()) {
        V3GraphEdge* ध्रुव inEdgep = avertexp->inEdges().frontp();
        V3GraphEdge* ध्रुव outEdgep = avertexp->outEdges().frontp();
        V3GraphVertex* ध्रुव inVertexp = inEdgep->fromp();
        V3GraphVertex* ध्रुव outVertexp = outEdgep->शीर्ष();
        // The in and out may be the same node; we'll make a loop
        // The in OR out may be THIS node; we can't delete it then.
        यदि (inVertexp != avertexp && outVertexp != avertexp) {
            UINFO(9, "  SimplifyOneRemove " << avertexp);
            avertexp->setDelete();  // Mark so we won't delete it twice
            // Make a new edge connecting the two vertices directly
            // If both are breakable, we pick the one with less weight, else it's arbitrary
            // We can forget about the origEdge list for the "non-selected" set of edges,
            // as we need to break only one set or the other set of edges, not both.
            // (This is why we must give preference to the cutable set.)
            V3GraphEdge* ध्रुव templateEdgep
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
            work_ढकला(inVertexp);
            work_ढकला(outVertexp);
        }
    }
}

शून्य GraphAcyc::simplifyOut(GraphAcycVertex* avertexp) {
    // If a node has one output that's not cutable, all its inputs can be reassigned
    // to the next node in the list
    यदि (avertexp->isDelete()) प्रत्यागम;
    यदि (avertexp->outSize1()) {
        V3GraphEdge* ध्रुव outEdgep = avertexp->outEdges().frontp();
        यदि (!outEdgep->cutable()) {
            V3GraphVertex* outVertexp = outEdgep->शीर्ष();
            UINFO(9, "  SimplifyOutRemove " << avertexp);
            avertexp->setDelete();  // Mark so we won't delete it twice
            प्रति (V3GraphEdge* ध्रुव inEdgep : avertexp->inEdges().unlinkable()) {
                V3GraphVertex* inVertexp = inEdgep->fromp();
                यदि (inVertexp == avertexp) {
                    यदि (debug()) v3error("Non-cutable vertex=" << avertexp);  // LCOV_EXCL_LINE
                    v3error("Circular logic when ordering code (non-cutable edge loop)\n"
                            << m_origGraphp->reportLoops(  // calls OrderGraph::loopsVertexCb
                                   &V3GraphEdge::followNotCutable, avertexp->origVertexp()));
                    // Things are unlikely to end well at this point,
                    // but we'll try something to get to further errors...
                    inEdgep->cutable(सत्य);
                    प्रत्यागम;
                }
                // Make a new edge connecting the two vertices directly
                // cppcheck-suppress leakReturnValNotUsed
                edgeFromEdge(inEdgep, inVertexp, outVertexp);
                // Remove old edge
                VL_DO_DANGLING(inEdgep->unlinkDelete(), inEdgep);
                work_ढकला(inVertexp);
            }
            VL_DO_DANGLING(outEdgep->unlinkDelete(), outEdgep);
            work_ढकला(outVertexp);
        }
    }
}

शून्य GraphAcyc::simplifyDup(GraphAcycVertex* avertexp) {
    // Remove redundant edges
    यदि (avertexp->isDelete()) प्रत्यागम;
    // Clear marks
    प्रति (V3GraphEdge& edge : avertexp->outEdges()) edge.शीर्ष()->userp(nullptr);
    // Mark edges and detect duplications
    प्रति (V3GraphEdge* ध्रुव edgep : avertexp->outEdges().unlinkable()) {
        V3GraphVertex* outVertexp = edgep->शीर्ष();
        V3GraphEdge* prevEdgep = static_cast<V3GraphEdge*>(outVertexp->userp());
        यदि (prevEdgep) {
            यदि (!prevEdgep->cutable()) {
                // !cutable duplicates prev !cutable: we can ignore it, redundant
                //  cutable duplicates prev !cutable: know it's not a relevant loop, ignore it
                UINFO(8, "    DelDupEdge " << avertexp << " -> " << edgep->शीर्ष());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            } अन्यथा यदि (!edgep->cutable()) {
                // !cutable duplicates prev  cutable: delete the earlier cutable
                UINFO(8, "    DelDupPrev " << avertexp << " -> " << prevEdgep->शीर्ष());
                VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                outVertexp->userp(edgep);
            } अन्यथा {
                //  cutable duplicates prev  cutable: combine weights
                UINFO(8, "    DelDupComb " << avertexp << " -> " << edgep->शीर्ष());
                prevEdgep->weight(prevEdgep->weight() + edgep->weight());
                addOrigEdgep(prevEdgep, edgep);
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            work_ढकला(outVertexp);
            work_ढकला(avertexp);
        } अन्यथा {
            // No previous assignment
            outVertexp->userp(edgep);
        }
    }
}

शून्य GraphAcyc::cutBasic(GraphAcycVertex* avertexp) {
    // Detect and cleanup any loops from node to itself
    यदि (avertexp->isDelete()) प्रत्यागम;
    प्रति (V3GraphEdge* ध्रुव edgep : avertexp->outEdges().unlinkable()) {
        यदि (edgep->cutable() && edgep->शीर्ष() == avertexp) {
            cutOrigEdge(edgep, "  Cut Basic");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_ढकला(avertexp);
        }
    }
}

शून्य GraphAcyc::cutBackward(GraphAcycVertex* avertexp) {
    // If a cutable edge is from A->B, and there's a non-cutable edge B->A, then must cut!
    यदि (avertexp->isDelete()) प्रत्यागम;
    // Clear marks
    प्रति (V3GraphEdge& edge : avertexp->outEdges()) edge.शीर्ष()->user(असत्य);
    प्रति (V3GraphEdge& edge : avertexp->inEdges()) {
        यदि (!edge.cutable()) edge.fromp()->user(सत्य);
    }
    // Detect duplications
    प्रति (V3GraphEdge* ध्रुव edgep : avertexp->outEdges().unlinkable()) {
        यदि (edgep->cutable() && edgep->शीर्ष()->user()) {
            cutOrigEdge(edgep, "  Cut A->B->A");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_ढकला(avertexp);
        }
    }
}

शून्य GraphAcyc::place() {
    // Input is m_breakGraph with ranks already assigned on non-breakable edges

    // Make a list of all cutable edges in the graph
    पूर्णाङ्क अंक_Edges = 0;
    प्रति (V3GraphVertex& vertex : m_breakGraph.vertices()) {
        प्रति (V3GraphEdge& edge : vertex.outEdges()) {
            यदि (edge.weight() && edge.cutable()) ++अंक_Edges;
        }
    }
    UINFO(4, "    Cutable edges = " << अंक_Edges);

    std::vector<V3GraphEdge*> edges;  // List of all edges to be processed
    // Make the vector properly sized right off the bat -- faster than reallocating
    edges.reserve(अंक_Edges + 1);
    प्रति (V3GraphVertex& vertex : m_breakGraph.vertices()) {
        vertex.user(0);  // Clear in prep of next step
        प्रति (V3GraphEdge& edge : vertex.outEdges()) {
            यदि (edge.weight() && edge.cutable()) edges.ढकला_back(&edge);
        }
    }

    // Sort by weight, then by vertex (so that we completely process one vertex, when possible)
    stable_sort(edges.begin(), edges.end(), GraphAcycEdgeCmp());

    // Process each edge in weighted order
    m_placeStep = 10;
    प्रति (V3GraphEdge* edgep : edges) placeTryEdge(edgep);
}

शून्य GraphAcyc::placeTryEdge(V3GraphEdge* edgep) {
    // Try to make this edge uncutable
    m_placeStep++;
    UINFO(8, "    PlaceEdge s" << m_placeStep << " w" << edgep->weight() << " " << edgep->fromp());
    // Make the edge uncutable so we detect it in placement
    edgep->cutable(असत्य);
    // Vertex::m_user begin: number indicates this edge was completed
    // Try to assign ranks, presuming this edge is in place
    // If we come across user()==placestep, we've detected a loop and must back out
    ध्रुव तार्किक loop
        = placeIterate(static_cast<GraphAcycVertex*>(edgep->शीर्ष()), edgep->fromp()->rank() + 1);
    यदि (!loop) {
        // No loop, we can keep it as uncutable
        // Commit the new ranks we calculated
        // Just cleanup the list.  If this is slow, we can add another set of
        // user counters to avoid cleaning up the list.
        यावत् (workBeginp()) work_काढा();
    } अन्यथा {
        // Adding this edge would cause a loop, kill it
        edgep->cutable(सत्य);  // So graph still looks pretty
        cutOrigEdge(edgep, "  Cut loop");
        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        // Back out the ranks we calculated
        यावत् (GraphAcycVertex* vertexp = workBeginp()) {
            work_काढा();
            vertexp->rank(vertexp->m_storedRank);
        }
    }
}

तार्किक GraphAcyc::placeIterate(GraphAcycVertex* vertexp, uint32_t currentRank) {
    // Assign rank to each unvisited node
    //   rank() is the "committed rank" of the graph known without loops
    // If larger rank is found, assign it and loop back through
    // If we hit a back node make a list of all loops
    यदि (vertexp->rank() >= currentRank) प्रत्यागम असत्य;  // Already processed it
    यदि (vertexp->user() == m_placeStep) प्रत्यागम सत्य;  // Loop detected
    vertexp->user(m_placeStep);
    // Remember we're changing the rank of this node; might need to back out
    यदि (!vertexp->m_onWorkList) {
        vertexp->m_storedRank = vertexp->rank();
        work_ढकला(vertexp);
    }
    vertexp->rank(currentRank);
    // Follow all edges and increase their ranks
    प्रति (V3GraphEdge& edge : vertexp->outEdges()) {
        यदि (edge.weight() && !edge.cutable()) {
            यदि (placeIterate(static_cast<GraphAcycVertex*>(edge.शीर्ष()), currentRank + 1)) {
                // We don't need to reset user(); we'll use a different placeStep for the next edge
                प्रत्यागम सत्य;  // Loop detected
            }
        }
    }
    vertexp->user(0);
    प्रत्यागम असत्य;
}

//----- Main algorithm entry point

शून्य GraphAcyc::main() {
    m_breakGraph.userClearEdges();

    // Color based on possible loops
    m_origGraphp->stronglyConnected(m_origEdgeFuncp);

    // Make a new graph with vertices that have only a single vertex
    // for each group of old vertices that are interconnected with unbreakable
    // edges (and thus can't represent loops - if we did the unbreakable
    // marking right, anyways)
    buildGraph(m_origGraphp);
    यदि (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_pre");

    // Perform simple optimizations before any cuttings
    simplify(असत्य);
    यदि (dumpGraphLevel() >= 5) m_breakGraph.dumpDotFilePrefixed("acyc_simp");

    UINFO(4, " Cutting trivial loops");
    simplify(सत्य);
    यदि (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_mid");

    UINFO(4, " Ranking");
    m_breakGraph.rank(&V3GraphEdge::followNotCutable);
    यदि (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_rank");

    UINFO(4, " Placement");
    place();
    यदि (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_place");

    UINFO(4, " Final Ranking");
    // Only needed to assert there are no loops in completed graph
    m_breakGraph.rank(&V3GraphEdge::followAlwaysTrue);
    यदि (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_done");
}

शून्य V3Graph::acyclic(V3EdgeFuncP edgeFuncp) {
    UINFO(4, "Acyclic");
    GraphAcyc acyc{एतत्, edgeFuncp};
    acyc.main();
    UINFO(4, "Acyclic done");
}
