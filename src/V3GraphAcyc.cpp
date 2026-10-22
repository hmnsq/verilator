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

#घोषणा VL_MT_DISABLED_CODE_UNIT 1

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Global.h"
#समावेश "V3Graph.h"

#समावेश <algorithm>
#समावेश <list>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - acyclic
//      Break the minimal number of backward edges to make the graph acyclic

श्रेणी GraphAcycVertex final : खुला V3GraphVertex {
    VL_RTTI_IMPL(GraphAcycVertex, V3GraphVertex)
    // user() is used for various sub-algorithm pieces
    V3GraphVertex* स्थिर m_origVertexp;  // Pointer to first vertex this represents
रक्षित:
    मित्र श्रेणी GraphAcyc;
    V3ListLinks<GraphAcycVertex> m_links;  // List links to store instances of this class
    uint32_t m_storedRank = 0;  // Rank held until commit to edge placement
    bool m_onWorkList = false;  // True if already on list of work to do
    bool m_deleted = false;  // True if deleted

निजी:
    V3ListLinks<GraphAcycVertex>& links() { वापस m_links; }

खुला:
    // List type to store instances of this class
    using List = V3List<GraphAcycVertex, &GraphAcycVertex::links>;

    GraphAcycVertex(V3Graph* graphp, V3GraphVertex* origVertexp)
        : V3GraphVertex{graphp}
        , m_origVertexp{origVertexp} {}
    ~GraphAcycVertex() override = शेष;
    V3GraphVertex* origVertexp() स्थिर { वापस m_origVertexp; }
    व्योम setDelete() { m_deleted = true; }
    bool isDelete() स्थिर { वापस m_deleted; }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string name() स्थिर override { वापस m_origVertexp->name(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string dotColor() स्थिर override { वापस m_origVertexp->dotColor(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    File_पंक्ति* fileline() स्थिर override { वापस m_origVertexp->fileline(); }
};

//--------------------------------------------------------------------

श्रेणी GraphAcycEdge final : खुला V3GraphEdge {
    VL_RTTI_IMPL(GraphAcycEdge, V3GraphEdge)
    // userp() is always used to point to the head original graph edge
निजी:
    using OrigEdgeList = std::list<V3GraphEdge*>;  // List of orig edges, see also GraphAcyc's decl
    V3GraphEdge* origEdgep() स्थिर {
        स्थिर OrigEdgeList* स्थिर oEListp = static_cast<OrigEdgeList*>(userp());
        UASSERT(oEListp, "No original edge associated with acyc edge " << यह);
        वापस (oEListp->front());
    }

खुला:
    GraphAcycEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णांक weight,
                  bool cutable = false)
        : V3GraphEdge{graphp, fromp, शीर्ष, weight, cutable} {}
    ~GraphAcycEdge() override = शेष;
    // yellow=we might still cut it, else oldEdge: yellowGreen=made uncutable, red=uncutable
    string dotColor() स्थिर override { वापस (cutable() ? "yellow" : origEdgep()->dotColor()); }
};

//--------------------------------------------------------------------

काष्ठा GraphAcycEdgeCmp final {
    bool चालक()(स्थिर V3GraphEdge* lhsp, स्थिर V3GraphEdge* rhsp) स्थिर {
        अगर (lhsp->weight() > rhsp->weight()) वापस true;  // LHS goes first
        अगर (lhsp->weight() < rhsp->weight()) वापस false;  // RHS goes first
        वापस false;
    }
};

//--------------------------------------------------------------------

// CLASSES
श्रेणी GraphAcyc final {
    using OrigEdgeList
        = std::list<V3GraphEdge*>;  // List of orig edges, see also GraphAcycEdge's decl
    // GRAPH USERS
    //  origGraph
    //    GraphVertex::user()   GraphAycVerted* New graph node
    //  m_breakGraph
    //    GraphEdge::user()     OrigEdgeList*   Old graph edges
    //    GraphVertex::user     bool            Detection of loops in simplifyDupIterate
    // MEMBERS
    V3Graph* स्थिर m_origGraphp;  // Original graph
    V3Graph m_breakGraph;  // Graph with only breakable edges represented
    GraphAcycVertex::List m_work;  // List of vertices with optimization work left
    std::vector<OrigEdgeList*> m_origEdgeDelp;  // List of deletions to do when done
    स्थिर V3EdgeFuncP
        m_origEdgeFuncp;  // Function that says we follow this edge (in original graph)
    uint32_t m_placeStep = 0;  // Number that user() must be equal to to indicate processing

    // METHODS
    व्योम buildGraph(V3Graph* origGraphp);
    व्योम buildGraphIterate(V3GraphVertex* overtexp, GraphAcycVertex* avertexp);
    व्योम simplify(bool allowCut);
    व्योम simplifyNone(GraphAcycVertex* avertexp);
    व्योम simplifyOne(GraphAcycVertex* avertexp);
    व्योम simplifyOut(GraphAcycVertex* avertexp);
    व्योम simplifyDup(GraphAcycVertex* avertexp);
    व्योम cutBasic(GraphAcycVertex* avertexp);
    व्योम cutBackward(GraphAcycVertex* avertexp);
    व्योम deleteMarked();
    व्योम place();
    व्योम placeTryEdge(V3GraphEdge* edgep);
    bool placeIterate(GraphAcycVertex* vertexp, uint32_t currentRank);

    bool origFollowEdge(V3GraphEdge* edgep) {
        वापस (edgep->weight() && (m_origEdgeFuncp)(edgep));
    }
    व्योम edgeFromEdge(V3GraphEdge* oldedgep, V3GraphVertex* fromp, V3GraphVertex* शीर्ष) {
        // Make new breakGraph edge, with old edge as a template
        GraphAcycEdge* स्थिर newEdgep = नया GraphAcycEdge{&m_breakGraph, fromp, शीर्ष,
                                                          oldedgep->weight(), oldedgep->cutable()};
        newEdgep->userp(oldedgep->userp());  // Keep pointer to OrigEdgeList
    }
    व्योम addOrigEdgep(V3GraphEdge* toEdgep, V3GraphEdge* addEdgep) {
        // Add addEdge (or it's list) to list of edges that break edge represents
        // Note addEdge may already have a bunch of similar linked edge representations.  Yuk.
        UASSERT(addEdgep, "Adding nullptr");
        अगर (!toEdgep->userp()) {
            OrigEdgeList* स्थिर oep = नया OrigEdgeList;
            m_origEdgeDelp.धकेलो_back(oep);
            toEdgep->userp(oep);
        }
        OrigEdgeList* स्थिर oEListp = static_cast<OrigEdgeList*>(toEdgep->userp());
        अगर (OrigEdgeList* स्थिर addListp = static_cast<OrigEdgeList*>(addEdgep->userp())) {
            क्रम (स्थिर स्वतः& itr : *addListp) oEListp->धकेलो_back(itr);
            addListp->सफा();  // Done with it
        } अन्यथा {
            oEListp->धकेलो_back(addEdgep);
        }
    }
    व्योम cutOrigEdge(V3GraphEdge* breakEdgep, स्थिर अक्षर* why) {
        // From the break edge, cut edges in original graph it represents
        UINFO(8, why << " CUT " << breakEdgep->fromp());
        breakEdgep->cut();
        स्थिर OrigEdgeList* स्थिर oEListp = static_cast<OrigEdgeList*>(breakEdgep->userp());
        अगर (!oEListp) {
            v3fatalSrc("No original edge associated with cutting edge " << breakEdgep);
        }
        // The breakGraph edge may represent multiple real edges; cut them all
        क्रम (स्थिर स्वतः& origEdgep : *oEListp) {
            origEdgep->cut();
            UINFO(8, "  " << why << "   " << origEdgep->fromp() << " ->" << origEdgep->शीर्ष());
        }
    }
    // Work Queue
    व्योम work_धकेलो(V3GraphVertex* vertexp) {
        GraphAcycVertex* स्थिर avertexp = static_cast<GraphAcycVertex*>(vertexp);
        // Add vertex to list of nodes needing further optimization trials
        अगर (!avertexp->m_onWorkList) {
            avertexp->m_onWorkList = true;
            m_work.linkBack(avertexp);
        }
    }
    GraphAcycVertex* workBeginp() { वापस m_work.frontp(); }
    व्योम work_निकालो() {
        GraphAcycVertex* स्थिर avertexp = workBeginp();
        avertexp->m_onWorkList = false;
        m_work.unlink(avertexp);
    }

खुला:
    // CONSTRUCTORS
    GraphAcyc(V3Graph* origGraphp, V3EdgeFuncP edgeFuncp)
        : m_origGraphp{origGraphp}
        , m_origEdgeFuncp{edgeFuncp} {}
    ~GraphAcyc() {
        क्रम (OrigEdgeList* ip : m_origEdgeDelp) मिटाओ ip;
        m_origEdgeDelp.सफा();
    }
    व्योम मुख्य();
};

//--------------------------------------------------------------------

व्योम GraphAcyc::buildGraph(V3Graph* origGraphp) {
    // Presumes the graph has been strongly ordered,
    // and thus there's a unique color if there are loops in this subgraph.

    // For each old node, make a new graph node for optimization
    origGraphp->userClearVertices();
    origGraphp->userClearEdges();
    क्रम (V3GraphVertex& overtex : origGraphp->vertices()) {
        अगर (overtex.color()) {
            GraphAcycVertex* स्थिर avertexp = नया GraphAcycVertex{&m_breakGraph, &overtex};
            overtex.userp(avertexp);  // Stash so can look up later
        }
    }

    // Build edges between logic vertices
    क्रम (V3GraphVertex& overtex : origGraphp->vertices()) {
        अगर (overtex.color()) {
            GraphAcycVertex* स्थिर avertexp = static_cast<GraphAcycVertex*>(overtex.userp());
            buildGraphIterate(&overtex, avertexp);
        }
    }
}

व्योम GraphAcyc::buildGraphIterate(V3GraphVertex* overtexp, GraphAcycVertex* avertexp) {
    // Make new edges
    क्रम (V3GraphEdge& edge : overtexp->outEdges()) {
        अगर (origFollowEdge(&edge)) {  // not cut
            स्थिर V3GraphVertex* toVertexp = edge.शीर्ष();
            अगर (toVertexp->color()) {
                GraphAcycVertex* स्थिर toAVertexp
                    = static_cast<GraphAcycVertex*>(toVertexp->userp());
                // Replicate the old edge into the new graph
                // There may be multiple edges between same pairs of vertices
                V3GraphEdge* breakEdgep = नया GraphAcycEdge{&m_breakGraph, avertexp, toAVertexp,
                                                            edge.weight(), edge.cutable()};
                addOrigEdgep(breakEdgep, &edge);  // So can find original edge
            }
        }
    }
}

व्योम GraphAcyc::simplify(bool allowCut) {
    // Add all nodes to list of work to do
    क्रम (V3GraphVertex& vertex : m_breakGraph.vertices()) work_धकेलो(&vertex);
    // Optimize till everything finished
    जबतक (GraphAcycVertex* vertexp = workBeginp()) {
        work_निकालो();
        simplifyNone(vertexp);
        simplifyOne(vertexp);
        simplifyOut(vertexp);
        simplifyDup(vertexp);
        अगर (allowCut) {
            // The main algorithm works without these, though slower
            // So if changing the main algorithm, comment these out for a test run
            अगर (v3Global.opt.fAcycSimp()) {
                cutBasic(vertexp);
                cutBackward(vertexp);
            }
        }
    }
    deleteMarked();
}

व्योम GraphAcyc::deleteMarked() {
    // Delete nodes marked for removal
    क्रम (V3GraphVertex* स्थिर vtxp : m_breakGraph.vertices().unlinkable()) {
        GraphAcycVertex* स्थिर avertexp = static_cast<GraphAcycVertex*>(vtxp);
        अगर (avertexp->isDelete()) {
            VL_DO_DANGLING(avertexp->unlinkDelete(&m_breakGraph), avertexp);
        }
    }
}

व्योम GraphAcyc::simplifyNone(GraphAcycVertex* avertexp) {
    // Don't need any vertices with no inputs, There's no way they can have a loop.
    // Likewise, vertices with no outputs
    अगर (avertexp->isDelete()) वापस;
    अगर (avertexp->inEmpty() || avertexp->outEmpty()) {
        UINFO(9, "  SimplifyNoneRemove " << avertexp);
        avertexp->setDelete();  // Mark so we won't delete it twice
        // Remove edges
        जबतक (V3GraphEdge* स्थिर edgep = avertexp->outEdges().frontp()) {
            work_धकेलो(edgep->शीर्ष());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        जबतक (V3GraphEdge* स्थिर edgep = avertexp->inEdges().frontp()) {
            work_धकेलो(edgep->fromp());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
}

व्योम GraphAcyc::simplifyOne(GraphAcycVertex* avertexp) {
    // If a node has one input and one output, we can remove it and change the edges
    अगर (avertexp->isDelete()) वापस;
    अगर (avertexp->inSize1() && avertexp->outSize1()) {
        V3GraphEdge* स्थिर inEdgep = avertexp->inEdges().frontp();
        V3GraphEdge* स्थिर outEdgep = avertexp->outEdges().frontp();
        V3GraphVertex* स्थिर inVertexp = inEdgep->fromp();
        V3GraphVertex* स्थिर outVertexp = outEdgep->शीर्ष();
        // The in and out may be the same node; we'll make a loop
        // The in OR out may be THIS node; we can't delete it then.
        अगर (inVertexp != avertexp && outVertexp != avertexp) {
            UINFO(9, "  SimplifyOneRemove " << avertexp);
            avertexp->setDelete();  // Mark so we won't delete it twice
            // Make a new edge connecting the two vertices directly
            // If both are breakable, we pick the one with less weight, else it's arbitrary
            // We can forget about the origEdge list for the "non-selected" set of edges,
            // as we need to break only one set or the other set of edges, not both.
            // (This is why we must give preference to the cutable set.)
            V3GraphEdge* स्थिर templateEdgep
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
            work_धकेलो(inVertexp);
            work_धकेलो(outVertexp);
        }
    }
}

व्योम GraphAcyc::simplifyOut(GraphAcycVertex* avertexp) {
    // If a node has one output that's not cutable, all its inputs can be reassigned
    // to the next node in the list
    अगर (avertexp->isDelete()) वापस;
    अगर (avertexp->outSize1()) {
        V3GraphEdge* स्थिर outEdgep = avertexp->outEdges().frontp();
        अगर (!outEdgep->cutable()) {
            V3GraphVertex* outVertexp = outEdgep->शीर्ष();
            UINFO(9, "  SimplifyOutRemove " << avertexp);
            avertexp->setDelete();  // Mark so we won't delete it twice
            क्रम (V3GraphEdge* स्थिर inEdgep : avertexp->inEdges().unlinkable()) {
                V3GraphVertex* inVertexp = inEdgep->fromp();
                अगर (inVertexp == avertexp) {
                    अगर (debug()) v3error("Non-cutable vertex=" << avertexp);  // LCOV_EXCL_LINE
                    v3error("Circular logic when ordering code (non-cutable edge loop)\n"
                            << m_origGraphp->reportLoops(  // calls OrderGraph::loopsVertexCb
                                   &V3GraphEdge::followNotCutable, avertexp->origVertexp()));
                    // Things are unlikely to end well at this point,
                    // but we'll try something to get to further errors...
                    inEdgep->cutable(true);
                    वापस;
                }
                // Make a new edge connecting the two vertices directly
                // cppcheck-suppress leakReturnValNotUsed
                edgeFromEdge(inEdgep, inVertexp, outVertexp);
                // Remove old edge
                VL_DO_DANGLING(inEdgep->unlinkDelete(), inEdgep);
                work_धकेलो(inVertexp);
            }
            VL_DO_DANGLING(outEdgep->unlinkDelete(), outEdgep);
            work_धकेलो(outVertexp);
        }
    }
}

व्योम GraphAcyc::simplifyDup(GraphAcycVertex* avertexp) {
    // Remove redundant edges
    अगर (avertexp->isDelete()) वापस;
    // Clear marks
    क्रम (V3GraphEdge& edge : avertexp->outEdges()) edge.शीर्ष()->userp(nullptr);
    // Mark edges and detect duplications
    क्रम (V3GraphEdge* स्थिर edgep : avertexp->outEdges().unlinkable()) {
        V3GraphVertex* outVertexp = edgep->शीर्ष();
        V3GraphEdge* पूर्व_Edgep = static_cast<V3GraphEdge*>(outVertexp->userp());
        अगर (पूर्व_Edgep) {
            अगर (!पूर्व_Edgep->cutable()) {
                // !cutable duplicates prev !cutable: we can ignore it, redundant
                //  cutable duplicates prev !cutable: know it's not a relevant loop, ignore it
                UINFO(8, "    DelDupEdge " << avertexp << " -> " << edgep->शीर्ष());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            } अन्यथा अगर (!edgep->cutable()) {
                // !cutable duplicates prev  cutable: delete the earlier cutable
                UINFO(8, "    DelDupPrev " << avertexp << " -> " << पूर्व_Edgep->शीर्ष());
                VL_DO_DANGLING(पूर्व_Edgep->unlinkDelete(), पूर्व_Edgep);
                outVertexp->userp(edgep);
            } अन्यथा {
                //  cutable duplicates prev  cutable: combine weights
                UINFO(8, "    DelDupComb " << avertexp << " -> " << edgep->शीर्ष());
                पूर्व_Edgep->weight(पूर्व_Edgep->weight() + edgep->weight());
                addOrigEdgep(पूर्व_Edgep, edgep);
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            work_धकेलो(outVertexp);
            work_धकेलो(avertexp);
        } अन्यथा {
            // No previous assignment
            outVertexp->userp(edgep);
        }
    }
}

व्योम GraphAcyc::cutBasic(GraphAcycVertex* avertexp) {
    // Detect and cleanup any loops from node to itself
    अगर (avertexp->isDelete()) वापस;
    क्रम (V3GraphEdge* स्थिर edgep : avertexp->outEdges().unlinkable()) {
        अगर (edgep->cutable() && edgep->शीर्ष() == avertexp) {
            cutOrigEdge(edgep, "  Cut Basic");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_धकेलो(avertexp);
        }
    }
}

व्योम GraphAcyc::cutBackward(GraphAcycVertex* avertexp) {
    // If a cutable edge is from A->B, and there's a non-cutable edge B->A, then must cut!
    अगर (avertexp->isDelete()) वापस;
    // Clear marks
    क्रम (V3GraphEdge& edge : avertexp->outEdges()) edge.शीर्ष()->user(false);
    क्रम (V3GraphEdge& edge : avertexp->inEdges()) {
        अगर (!edge.cutable()) edge.fromp()->user(true);
    }
    // Detect duplications
    क्रम (V3GraphEdge* स्थिर edgep : avertexp->outEdges().unlinkable()) {
        अगर (edgep->cutable() && edgep->शीर्ष()->user()) {
            cutOrigEdge(edgep, "  Cut A->B->A");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_धकेलो(avertexp);
        }
    }
}

व्योम GraphAcyc::place() {
    // Input is m_breakGraph with ranks already assigned on non-breakable edges

    // Make a list of all cutable edges in the graph
    पूर्णांक अंक_Edges = 0;
    क्रम (V3GraphVertex& vertex : m_breakGraph.vertices()) {
        क्रम (V3GraphEdge& edge : vertex.outEdges()) {
            अगर (edge.weight() && edge.cutable()) ++अंक_Edges;
        }
    }
    UINFO(4, "    Cutable edges = " << अंक_Edges);

    std::vector<V3GraphEdge*> edges;  // List of all edges to be processed
    // Make the vector properly sized right off the bat -- faster than reallocating
    edges.reserve(अंक_Edges + 1);
    क्रम (V3GraphVertex& vertex : m_breakGraph.vertices()) {
        vertex.user(0);  // Clear in prep of next step
        क्रम (V3GraphEdge& edge : vertex.outEdges()) {
            अगर (edge.weight() && edge.cutable()) edges.धकेलो_back(&edge);
        }
    }

    // Sort by weight, then by vertex (so that we completely process one vertex, when possible)
    stable_sort(edges.begin(), edges.end(), GraphAcycEdgeCmp());

    // Process each edge in weighted order
    m_placeStep = 10;
    क्रम (V3GraphEdge* edgep : edges) placeTryEdge(edgep);
}

व्योम GraphAcyc::placeTryEdge(V3GraphEdge* edgep) {
    // Try to make this edge uncutable
    m_placeStep++;
    UINFO(8, "    PlaceEdge s" << m_placeStep << " w" << edgep->weight() << " " << edgep->fromp());
    // Make the edge uncutable so we detect it in placement
    edgep->cutable(false);
    // Vertex::m_user begin: number indicates this edge was completed
    // Try to assign ranks, presuming this edge is in place
    // If we come across user()==placestep, we've detected a loop and must back out
    स्थिर bool loop
        = placeIterate(static_cast<GraphAcycVertex*>(edgep->शीर्ष()), edgep->fromp()->rank() + 1);
    अगर (!loop) {
        // No loop, we can keep it as uncutable
        // Commit the new ranks we calculated
        // Just cleanup the list.  If this is slow, we can add another set of
        // user counters to avoid cleaning up the list.
        जबतक (workBeginp()) work_निकालो();
    } अन्यथा {
        // Adding this edge would cause a loop, kill it
        edgep->cutable(true);  // So graph still looks pretty
        cutOrigEdge(edgep, "  Cut loop");
        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        // Back out the ranks we calculated
        जबतक (GraphAcycVertex* vertexp = workBeginp()) {
            work_निकालो();
            vertexp->rank(vertexp->m_storedRank);
        }
    }
}

bool GraphAcyc::placeIterate(GraphAcycVertex* vertexp, uint32_t currentRank) {
    // Assign rank to each unvisited node
    //   rank() is the "committed rank" of the graph known without loops
    // If larger rank is found, assign it and loop back through
    // If we hit a back node make a list of all loops
    अगर (vertexp->rank() >= currentRank) वापस false;  // Already processed it
    अगर (vertexp->user() == m_placeStep) वापस true;  // Loop detected
    vertexp->user(m_placeStep);
    // Remember we're changing the rank of this node; might need to back out
    अगर (!vertexp->m_onWorkList) {
        vertexp->m_storedRank = vertexp->rank();
        work_धकेलो(vertexp);
    }
    vertexp->rank(currentRank);
    // Follow all edges and increase their ranks
    क्रम (V3GraphEdge& edge : vertexp->outEdges()) {
        अगर (edge.weight() && !edge.cutable()) {
            अगर (placeIterate(static_cast<GraphAcycVertex*>(edge.शीर्ष()), currentRank + 1)) {
                // We don't need to reset user(); we'll use a different placeStep for the next edge
                वापस true;  // Loop detected
            }
        }
    }
    vertexp->user(0);
    वापस false;
}

//----- Main algorithm entry point

व्योम GraphAcyc::मुख्य() {
    m_breakGraph.userClearEdges();

    // Color based on possible loops
    m_origGraphp->stronglyConnected(m_origEdgeFuncp);

    // Make a new graph with vertices that have only a single vertex
    // for each group of old vertices that are interconnected with unbreakable
    // edges (and thus can't represent loops - if we did the unbreakable
    // marking right, anyways)
    buildGraph(m_origGraphp);
    अगर (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_pre");

    // Perform simple optimizations before any cuttings
    simplify(false);
    अगर (dumpGraphLevel() >= 5) m_breakGraph.dumpDotFilePrefixed("acyc_simp");

    UINFO(4, " Cutting trivial loops");
    simplify(true);
    अगर (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_mid");

    UINFO(4, " Ranking");
    m_breakGraph.rank(&V3GraphEdge::followNotCutable);
    अगर (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_rank");

    UINFO(4, " Placement");
    place();
    अगर (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_place");

    UINFO(4, " Final Ranking");
    // Only needed to assert there are no loops in completed graph
    m_breakGraph.rank(&V3GraphEdge::followAlwaysTrue);
    अगर (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_done");
}

व्योम V3Graph::acyclic(V3EdgeFuncP edgeFuncp) {
    UINFO(4, "Acyclic");
    GraphAcyc acyc{यह, edgeFuncp};
    acyc.मुख्य();
    UINFO(4, "Acyclic done");
}
