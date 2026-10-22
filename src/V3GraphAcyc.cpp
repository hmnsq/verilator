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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3Graph.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <list>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - acyclic
//      Break the minimal number of backward edges to make the graph acyclic

வகுப்பு GraphAcycVertex final : பொது V3GraphVertex {
    VL_RTTI_IMPL(GraphAcycVertex, V3GraphVertex)
    // user() is used for various sub-algorithm pieces
    V3GraphVertex* மாறிலி m_origVertexp;  // Pointer to first vertex this represents
பாதுகாக்கப்பட்ட:
    friend வகுப்பு GraphAcyc;
    V3ListLinks<GraphAcycVertex> m_links;  // List links to store instances of this class
    uint32_t m_storedRank = 0;  // Rank held until commit to edge placement
    தருக்க m_onWorkList = பொய்;  // True if already on list of work to do
    தருக்க m_deleted = பொய்;  // True if deleted

தனிப்பட்ட:
    V3ListLinks<GraphAcycVertex>& links() { திரும்பு m_links; }

பொது:
    // List type to store instances of this class
    using List = V3List<GraphAcycVertex, &GraphAcycVertex::links>;

    GraphAcycVertex(V3Graph* graphp, V3GraphVertex* origVertexp)
        : V3GraphVertex{graphp}
        , m_origVertexp{origVertexp} {}
    ~GraphAcycVertex() override = இயல்பு;
    V3GraphVertex* origVertexp() மாறிலி { திரும்பு m_origVertexp; }
    வெறுமை setDelete() { m_deleted = உண்மை; }
    தருக்க isDelete() மாறிலி { திரும்பு m_deleted; }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string name() மாறிலி override { திரும்பு m_origVertexp->name(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string dotColor() மாறிலி override { திரும்பு m_origVertexp->dotColor(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    File_வரி* fileline() மாறிலி override { திரும்பு m_origVertexp->fileline(); }
};

//--------------------------------------------------------------------

வகுப்பு GraphAcycEdge final : பொது V3GraphEdge {
    VL_RTTI_IMPL(GraphAcycEdge, V3GraphEdge)
    // userp() is always used to point to the head original graph edge
தனிப்பட்ட:
    using OrigEdgeList = std::list<V3GraphEdge*>;  // List of orig edges, see also GraphAcyc's decl
    V3GraphEdge* origEdgep() மாறிலி {
        மாறிலி OrigEdgeList* மாறிலி oEListp = static_cast<OrigEdgeList*>(userp());
        UASSERT(oEListp, "No original edge associated with acyc edge " << இது);
        திரும்பு (oEListp->front());
    }

பொது:
    GraphAcycEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* உச்சி, முழு weight,
                  தருக்க cutable = பொய்)
        : V3GraphEdge{graphp, fromp, உச்சி, weight, cutable} {}
    ~GraphAcycEdge() override = இயல்பு;
    // yellow=we might still cut it, else oldEdge: yellowGreen=made uncutable, red=uncutable
    string dotColor() மாறிலி override { திரும்பு (cutable() ? "yellow" : origEdgep()->dotColor()); }
};

//--------------------------------------------------------------------

கட்டமைப்பு GraphAcycEdgeCmp final {
    தருக்க operator()(மாறிலி V3GraphEdge* lhsp, மாறிலி V3GraphEdge* rhsp) மாறிலி {
        எனில் (lhsp->weight() > rhsp->weight()) திரும்பு உண்மை;  // LHS goes first
        எனில் (lhsp->weight() < rhsp->weight()) திரும்பு பொய்;  // RHS goes first
        திரும்பு பொய்;
    }
};

//--------------------------------------------------------------------

// CLASSES
வகுப்பு GraphAcyc final {
    using OrigEdgeList
        = std::list<V3GraphEdge*>;  // List of orig edges, see also GraphAcycEdge's decl
    // GRAPH USERS
    //  origGraph
    //    GraphVertex::user()   GraphAycVerted* New graph node
    //  m_breakGraph
    //    GraphEdge::user()     OrigEdgeList*   Old graph edges
    //    GraphVertex::user     bool            Detection of loops in simplifyDupIterate
    // MEMBERS
    V3Graph* மாறிலி m_origGraphp;  // Original graph
    V3Graph m_breakGraph;  // Graph with only breakable edges represented
    GraphAcycVertex::List m_work;  // List of vertices with optimization work left
    std::vector<OrigEdgeList*> m_origEdgeDelp;  // List of deletions to do when done
    மாறிலி V3EdgeFuncP
        m_origEdgeFuncp;  // Function that says we follow this edge (in original graph)
    uint32_t m_placeStep = 0;  // Number that user() must be equal to to indicate processing

    // METHODS
    வெறுமை buildGraph(V3Graph* origGraphp);
    வெறுமை buildGraphIterate(V3GraphVertex* overtexp, GraphAcycVertex* avertexp);
    வெறுமை simplify(தருக்க allowCut);
    வெறுமை simplifyNone(GraphAcycVertex* avertexp);
    வெறுமை simplifyOne(GraphAcycVertex* avertexp);
    வெறுமை simplifyOut(GraphAcycVertex* avertexp);
    வெறுமை simplifyDup(GraphAcycVertex* avertexp);
    வெறுமை cutBasic(GraphAcycVertex* avertexp);
    வெறுமை cutBackward(GraphAcycVertex* avertexp);
    வெறுமை deleteMarked();
    வெறுமை place();
    வெறுமை placeTryEdge(V3GraphEdge* edgep);
    தருக்க placeIterate(GraphAcycVertex* vertexp, uint32_t currentRank);

    தருக்க origFollowEdge(V3GraphEdge* edgep) {
        திரும்பு (edgep->weight() && (m_origEdgeFuncp)(edgep));
    }
    வெறுமை edgeFromEdge(V3GraphEdge* oldedgep, V3GraphVertex* fromp, V3GraphVertex* உச்சி) {
        // Make new breakGraph edge, with old edge as a template
        GraphAcycEdge* மாறிலி newEdgep = புதிய GraphAcycEdge{&m_breakGraph, fromp, உச்சி,
                                                          oldedgep->weight(), oldedgep->cutable()};
        newEdgep->userp(oldedgep->userp());  // Keep pointer to OrigEdgeList
    }
    வெறுமை addOrigEdgep(V3GraphEdge* toEdgep, V3GraphEdge* addEdgep) {
        // Add addEdge (or it's list) to list of edges that break edge represents
        // Note addEdge may already have a bunch of similar linked edge representations.  Yuk.
        UASSERT(addEdgep, "Adding nullptr");
        எனில் (!toEdgep->userp()) {
            OrigEdgeList* மாறிலி oep = புதிய OrigEdgeList;
            m_origEdgeDelp.தள்ளு_back(oep);
            toEdgep->userp(oep);
        }
        OrigEdgeList* மாறிலி oEListp = static_cast<OrigEdgeList*>(toEdgep->userp());
        எனில் (OrigEdgeList* மாறிலி addListp = static_cast<OrigEdgeList*>(addEdgep->userp())) {
            க்கு (மாறிலி auto& itr : *addListp) oEListp->தள்ளு_back(itr);
            addListp->clear();  // Done with it
        } இல்லையெனில் {
            oEListp->தள்ளு_back(addEdgep);
        }
    }
    வெறுமை cutOrigEdge(V3GraphEdge* breakEdgep, மாறிலி எழுத்து* why) {
        // From the break edge, cut edges in original graph it represents
        UINFO(8, why << " CUT " << breakEdgep->fromp());
        breakEdgep->cut();
        மாறிலி OrigEdgeList* மாறிலி oEListp = static_cast<OrigEdgeList*>(breakEdgep->userp());
        எனில் (!oEListp) {
            v3fatalSrc("No original edge associated with cutting edge " << breakEdgep);
        }
        // The breakGraph edge may represent multiple real edges; cut them all
        க்கு (மாறிலி auto& origEdgep : *oEListp) {
            origEdgep->cut();
            UINFO(8, "  " << why << "   " << origEdgep->fromp() << " ->" << origEdgep->உச்சி());
        }
    }
    // Work Queue
    வெறுமை work_தள்ளு(V3GraphVertex* vertexp) {
        GraphAcycVertex* மாறிலி avertexp = static_cast<GraphAcycVertex*>(vertexp);
        // Add vertex to list of nodes needing further optimization trials
        எனில் (!avertexp->m_onWorkList) {
            avertexp->m_onWorkList = உண்மை;
            m_work.linkBack(avertexp);
        }
    }
    GraphAcycVertex* workBeginp() { திரும்பு m_work.frontp(); }
    வெறுமை work_எடு() {
        GraphAcycVertex* மாறிலி avertexp = workBeginp();
        avertexp->m_onWorkList = பொய்;
        m_work.unlink(avertexp);
    }

பொது:
    // CONSTRUCTORS
    GraphAcyc(V3Graph* origGraphp, V3EdgeFuncP edgeFuncp)
        : m_origGraphp{origGraphp}
        , m_origEdgeFuncp{edgeFuncp} {}
    ~GraphAcyc() {
        க்கு (OrigEdgeList* ip : m_origEdgeDelp) delete ip;
        m_origEdgeDelp.clear();
    }
    வெறுமை முதன்மை();
};

//--------------------------------------------------------------------

வெறுமை GraphAcyc::buildGraph(V3Graph* origGraphp) {
    // Presumes the graph has been strongly ordered,
    // and thus there's a unique color if there are loops in this subgraph.

    // For each old node, make a new graph node for optimization
    origGraphp->userClearVertices();
    origGraphp->userClearEdges();
    க்கு (V3GraphVertex& overtex : origGraphp->vertices()) {
        எனில் (overtex.color()) {
            GraphAcycVertex* மாறிலி avertexp = புதிய GraphAcycVertex{&m_breakGraph, &overtex};
            overtex.userp(avertexp);  // Stash so can look up later
        }
    }

    // Build edges between logic vertices
    க்கு (V3GraphVertex& overtex : origGraphp->vertices()) {
        எனில் (overtex.color()) {
            GraphAcycVertex* மாறிலி avertexp = static_cast<GraphAcycVertex*>(overtex.userp());
            buildGraphIterate(&overtex, avertexp);
        }
    }
}

வெறுமை GraphAcyc::buildGraphIterate(V3GraphVertex* overtexp, GraphAcycVertex* avertexp) {
    // Make new edges
    க்கு (V3GraphEdge& edge : overtexp->outEdges()) {
        எனில் (origFollowEdge(&edge)) {  // not cut
            மாறிலி V3GraphVertex* toVertexp = edge.உச்சி();
            எனில் (toVertexp->color()) {
                GraphAcycVertex* மாறிலி toAVertexp
                    = static_cast<GraphAcycVertex*>(toVertexp->userp());
                // Replicate the old edge into the new graph
                // There may be multiple edges between same pairs of vertices
                V3GraphEdge* breakEdgep = புதிய GraphAcycEdge{&m_breakGraph, avertexp, toAVertexp,
                                                            edge.weight(), edge.cutable()};
                addOrigEdgep(breakEdgep, &edge);  // So can find original edge
            }
        }
    }
}

வெறுமை GraphAcyc::simplify(தருக்க allowCut) {
    // Add all nodes to list of work to do
    க்கு (V3GraphVertex& vertex : m_breakGraph.vertices()) work_தள்ளு(&vertex);
    // Optimize till everything finished
    போது (GraphAcycVertex* vertexp = workBeginp()) {
        work_எடு();
        simplifyNone(vertexp);
        simplifyOne(vertexp);
        simplifyOut(vertexp);
        simplifyDup(vertexp);
        எனில் (allowCut) {
            // The main algorithm works without these, though slower
            // So if changing the main algorithm, comment these out for a test run
            எனில் (v3Global.opt.fAcycSimp()) {
                cutBasic(vertexp);
                cutBackward(vertexp);
            }
        }
    }
    deleteMarked();
}

வெறுமை GraphAcyc::deleteMarked() {
    // Delete nodes marked for removal
    க்கு (V3GraphVertex* மாறிலி vtxp : m_breakGraph.vertices().unlinkable()) {
        GraphAcycVertex* மாறிலி avertexp = static_cast<GraphAcycVertex*>(vtxp);
        எனில் (avertexp->isDelete()) {
            VL_DO_DANGLING(avertexp->unlinkDelete(&m_breakGraph), avertexp);
        }
    }
}

வெறுமை GraphAcyc::simplifyNone(GraphAcycVertex* avertexp) {
    // Don't need any vertices with no inputs, There's no way they can have a loop.
    // Likewise, vertices with no outputs
    எனில் (avertexp->isDelete()) திரும்பு;
    எனில் (avertexp->inEmpty() || avertexp->outEmpty()) {
        UINFO(9, "  SimplifyNoneRemove " << avertexp);
        avertexp->setDelete();  // Mark so we won't delete it twice
        // Remove edges
        போது (V3GraphEdge* மாறிலி edgep = avertexp->outEdges().frontp()) {
            work_தள்ளு(edgep->உச்சி());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        போது (V3GraphEdge* மாறிலி edgep = avertexp->inEdges().frontp()) {
            work_தள்ளு(edgep->fromp());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
}

வெறுமை GraphAcyc::simplifyOne(GraphAcycVertex* avertexp) {
    // If a node has one input and one output, we can remove it and change the edges
    எனில் (avertexp->isDelete()) திரும்பு;
    எனில் (avertexp->inSize1() && avertexp->outSize1()) {
        V3GraphEdge* மாறிலி inEdgep = avertexp->inEdges().frontp();
        V3GraphEdge* மாறிலி outEdgep = avertexp->outEdges().frontp();
        V3GraphVertex* மாறிலி inVertexp = inEdgep->fromp();
        V3GraphVertex* மாறிலி outVertexp = outEdgep->உச்சி();
        // The in and out may be the same node; we'll make a loop
        // The in OR out may be THIS node; we can't delete it then.
        எனில் (inVertexp != avertexp && outVertexp != avertexp) {
            UINFO(9, "  SimplifyOneRemove " << avertexp);
            avertexp->setDelete();  // Mark so we won't delete it twice
            // Make a new edge connecting the two vertices directly
            // If both are breakable, we pick the one with less weight, else it's arbitrary
            // We can forget about the origEdge list for the "non-selected" set of edges,
            // as we need to break only one set or the other set of edges, not both.
            // (This is why we must give preference to the cutable set.)
            V3GraphEdge* மாறிலி templateEdgep
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
            work_தள்ளு(inVertexp);
            work_தள்ளு(outVertexp);
        }
    }
}

வெறுமை GraphAcyc::simplifyOut(GraphAcycVertex* avertexp) {
    // If a node has one output that's not cutable, all its inputs can be reassigned
    // to the next node in the list
    எனில் (avertexp->isDelete()) திரும்பு;
    எனில் (avertexp->outSize1()) {
        V3GraphEdge* மாறிலி outEdgep = avertexp->outEdges().frontp();
        எனில் (!outEdgep->cutable()) {
            V3GraphVertex* outVertexp = outEdgep->உச்சி();
            UINFO(9, "  SimplifyOutRemove " << avertexp);
            avertexp->setDelete();  // Mark so we won't delete it twice
            க்கு (V3GraphEdge* மாறிலி inEdgep : avertexp->inEdges().unlinkable()) {
                V3GraphVertex* inVertexp = inEdgep->fromp();
                எனில் (inVertexp == avertexp) {
                    எனில் (debug()) v3error("Non-cutable vertex=" << avertexp);  // LCOV_EXCL_LINE
                    v3error("Circular logic when ordering code (non-cutable edge loop)\n"
                            << m_origGraphp->reportLoops(  // calls OrderGraph::loopsVertexCb
                                   &V3GraphEdge::followNotCutable, avertexp->origVertexp()));
                    // Things are unlikely to end well at this point,
                    // but we'll try something to get to further errors...
                    inEdgep->cutable(உண்மை);
                    திரும்பு;
                }
                // Make a new edge connecting the two vertices directly
                // cppcheck-suppress leakReturnValNotUsed
                edgeFromEdge(inEdgep, inVertexp, outVertexp);
                // Remove old edge
                VL_DO_DANGLING(inEdgep->unlinkDelete(), inEdgep);
                work_தள்ளு(inVertexp);
            }
            VL_DO_DANGLING(outEdgep->unlinkDelete(), outEdgep);
            work_தள்ளு(outVertexp);
        }
    }
}

வெறுமை GraphAcyc::simplifyDup(GraphAcycVertex* avertexp) {
    // Remove redundant edges
    எனில் (avertexp->isDelete()) திரும்பு;
    // Clear marks
    க்கு (V3GraphEdge& edge : avertexp->outEdges()) edge.உச்சி()->userp(nullptr);
    // Mark edges and detect duplications
    க்கு (V3GraphEdge* மாறிலி edgep : avertexp->outEdges().unlinkable()) {
        V3GraphVertex* outVertexp = edgep->உச்சி();
        V3GraphEdge* prevEdgep = static_cast<V3GraphEdge*>(outVertexp->userp());
        எனில் (prevEdgep) {
            எனில் (!prevEdgep->cutable()) {
                // !cutable duplicates prev !cutable: we can ignore it, redundant
                //  cutable duplicates prev !cutable: know it's not a relevant loop, ignore it
                UINFO(8, "    DelDupEdge " << avertexp << " -> " << edgep->உச்சி());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            } இல்லையெனில் எனில் (!edgep->cutable()) {
                // !cutable duplicates prev  cutable: delete the earlier cutable
                UINFO(8, "    DelDupPrev " << avertexp << " -> " << prevEdgep->உச்சி());
                VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                outVertexp->userp(edgep);
            } இல்லையெனில் {
                //  cutable duplicates prev  cutable: combine weights
                UINFO(8, "    DelDupComb " << avertexp << " -> " << edgep->உச்சி());
                prevEdgep->weight(prevEdgep->weight() + edgep->weight());
                addOrigEdgep(prevEdgep, edgep);
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            work_தள்ளு(outVertexp);
            work_தள்ளு(avertexp);
        } இல்லையெனில் {
            // No previous assignment
            outVertexp->userp(edgep);
        }
    }
}

வெறுமை GraphAcyc::cutBasic(GraphAcycVertex* avertexp) {
    // Detect and cleanup any loops from node to itself
    எனில் (avertexp->isDelete()) திரும்பு;
    க்கு (V3GraphEdge* மாறிலி edgep : avertexp->outEdges().unlinkable()) {
        எனில் (edgep->cutable() && edgep->உச்சி() == avertexp) {
            cutOrigEdge(edgep, "  Cut Basic");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_தள்ளு(avertexp);
        }
    }
}

வெறுமை GraphAcyc::cutBackward(GraphAcycVertex* avertexp) {
    // If a cutable edge is from A->B, and there's a non-cutable edge B->A, then must cut!
    எனில் (avertexp->isDelete()) திரும்பு;
    // Clear marks
    க்கு (V3GraphEdge& edge : avertexp->outEdges()) edge.உச்சி()->user(பொய்);
    க்கு (V3GraphEdge& edge : avertexp->inEdges()) {
        எனில் (!edge.cutable()) edge.fromp()->user(உண்மை);
    }
    // Detect duplications
    க்கு (V3GraphEdge* மாறிலி edgep : avertexp->outEdges().unlinkable()) {
        எனில் (edgep->cutable() && edgep->உச்சி()->user()) {
            cutOrigEdge(edgep, "  Cut A->B->A");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_தள்ளு(avertexp);
        }
    }
}

வெறுமை GraphAcyc::place() {
    // Input is m_breakGraph with ranks already assigned on non-breakable edges

    // Make a list of all cutable edges in the graph
    முழு இலக்கம்_Edges = 0;
    க்கு (V3GraphVertex& vertex : m_breakGraph.vertices()) {
        க்கு (V3GraphEdge& edge : vertex.outEdges()) {
            எனில் (edge.weight() && edge.cutable()) ++இலக்கம்_Edges;
        }
    }
    UINFO(4, "    Cutable edges = " << இலக்கம்_Edges);

    std::vector<V3GraphEdge*> edges;  // List of all edges to be processed
    // Make the vector properly sized right off the bat -- faster than reallocating
    edges.reserve(இலக்கம்_Edges + 1);
    க்கு (V3GraphVertex& vertex : m_breakGraph.vertices()) {
        vertex.user(0);  // Clear in prep of next step
        க்கு (V3GraphEdge& edge : vertex.outEdges()) {
            எனில் (edge.weight() && edge.cutable()) edges.தள்ளு_back(&edge);
        }
    }

    // Sort by weight, then by vertex (so that we completely process one vertex, when possible)
    stable_sort(edges.begin(), edges.end(), GraphAcycEdgeCmp());

    // Process each edge in weighted order
    m_placeStep = 10;
    க்கு (V3GraphEdge* edgep : edges) placeTryEdge(edgep);
}

வெறுமை GraphAcyc::placeTryEdge(V3GraphEdge* edgep) {
    // Try to make this edge uncutable
    m_placeStep++;
    UINFO(8, "    PlaceEdge s" << m_placeStep << " w" << edgep->weight() << " " << edgep->fromp());
    // Make the edge uncutable so we detect it in placement
    edgep->cutable(பொய்);
    // Vertex::m_user begin: number indicates this edge was completed
    // Try to assign ranks, presuming this edge is in place
    // If we come across user()==placestep, we've detected a loop and must back out
    மாறிலி தருக்க loop
        = placeIterate(static_cast<GraphAcycVertex*>(edgep->உச்சி()), edgep->fromp()->rank() + 1);
    எனில் (!loop) {
        // No loop, we can keep it as uncutable
        // Commit the new ranks we calculated
        // Just cleanup the list.  If this is slow, we can add another set of
        // user counters to avoid cleaning up the list.
        போது (workBeginp()) work_எடு();
    } இல்லையெனில் {
        // Adding this edge would cause a loop, kill it
        edgep->cutable(உண்மை);  // So graph still looks pretty
        cutOrigEdge(edgep, "  Cut loop");
        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        // Back out the ranks we calculated
        போது (GraphAcycVertex* vertexp = workBeginp()) {
            work_எடு();
            vertexp->rank(vertexp->m_storedRank);
        }
    }
}

தருக்க GraphAcyc::placeIterate(GraphAcycVertex* vertexp, uint32_t currentRank) {
    // Assign rank to each unvisited node
    //   rank() is the "committed rank" of the graph known without loops
    // If larger rank is found, assign it and loop back through
    // If we hit a back node make a list of all loops
    எனில் (vertexp->rank() >= currentRank) திரும்பு பொய்;  // Already processed it
    எனில் (vertexp->user() == m_placeStep) திரும்பு உண்மை;  // Loop detected
    vertexp->user(m_placeStep);
    // Remember we're changing the rank of this node; might need to back out
    எனில் (!vertexp->m_onWorkList) {
        vertexp->m_storedRank = vertexp->rank();
        work_தள்ளு(vertexp);
    }
    vertexp->rank(currentRank);
    // Follow all edges and increase their ranks
    க்கு (V3GraphEdge& edge : vertexp->outEdges()) {
        எனில் (edge.weight() && !edge.cutable()) {
            எனில் (placeIterate(static_cast<GraphAcycVertex*>(edge.உச்சி()), currentRank + 1)) {
                // We don't need to reset user(); we'll use a different placeStep for the next edge
                திரும்பு உண்மை;  // Loop detected
            }
        }
    }
    vertexp->user(0);
    திரும்பு பொய்;
}

//----- Main algorithm entry point

வெறுமை GraphAcyc::முதன்மை() {
    m_breakGraph.userClearEdges();

    // Color based on possible loops
    m_origGraphp->stronglyConnected(m_origEdgeFuncp);

    // Make a new graph with vertices that have only a single vertex
    // for each group of old vertices that are interconnected with unbreakable
    // edges (and thus can't represent loops - if we did the unbreakable
    // marking right, anyways)
    buildGraph(m_origGraphp);
    எனில் (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_pre");

    // Perform simple optimizations before any cuttings
    simplify(பொய்);
    எனில் (dumpGraphLevel() >= 5) m_breakGraph.dumpDotFilePrefixed("acyc_simp");

    UINFO(4, " Cutting trivial loops");
    simplify(உண்மை);
    எனில் (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_mid");

    UINFO(4, " Ranking");
    m_breakGraph.rank(&V3GraphEdge::followNotCutable);
    எனில் (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_rank");

    UINFO(4, " Placement");
    place();
    எனில் (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_place");

    UINFO(4, " Final Ranking");
    // Only needed to assert there are no loops in completed graph
    m_breakGraph.rank(&V3GraphEdge::followAlwaysTrue);
    எனில் (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_done");
}

வெறுமை V3Graph::acyclic(V3EdgeFuncP edgeFuncp) {
    UINFO(4, "Acyclic");
    GraphAcyc acyc{இது, edgeFuncp};
    acyc.முதன்மை();
    UINFO(4, "Acyclic done");
}
