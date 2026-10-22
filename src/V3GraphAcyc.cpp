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

رده GraphAcycVertex final : عمومی V3GraphVertex {
    VL_RTTI_IMPL(GraphAcycVertex, V3GraphVertex)
    // user() is used for various sub-algorithm pieces
    V3GraphVertex* ثابت m_origVertexp;  // Pointer to first vertex this represents
محافظتشده:
    friend رده GraphAcyc;
    V3ListLinks<GraphAcycVertex> m_links;  // List links to store instances of this class
    uint32_t m_storedRank = 0;  // Rank held until commit to edge placement
    منطقی m_onWorkList = نادرست;  // True if already on list of work to do
    منطقی m_deleted = نادرست;  // True if deleted

خصوصی:
    V3ListLinks<GraphAcycVertex>& links() { بازگشت m_links; }

عمومی:
    // List type to store instances of this class
    using List = V3List<GraphAcycVertex, &GraphAcycVertex::links>;

    GraphAcycVertex(V3Graph* graphp, V3GraphVertex* origVertexp)
        : V3GraphVertex{graphp}
        , m_origVertexp{origVertexp} {}
    ~GraphAcycVertex() override = پیشفرض;
    V3GraphVertex* origVertexp() ثابت { بازگشت m_origVertexp; }
    تهی setDelete() { m_deleted = درست; }
    منطقی isDelete() ثابت { بازگشت m_deleted; }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string name() ثابت override { بازگشت m_origVertexp->name(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string dotColor() ثابت override { بازگشت m_origVertexp->dotColor(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    File_سطر* fileline() ثابت override { بازگشت m_origVertexp->fileline(); }
};

//--------------------------------------------------------------------

رده GraphAcycEdge final : عمومی V3GraphEdge {
    VL_RTTI_IMPL(GraphAcycEdge, V3GraphEdge)
    // userp() is always used to point to the head original graph edge
خصوصی:
    using OrigEdgeList = std::list<V3GraphEdge*>;  // List of orig edges, see also GraphAcyc's decl
    V3GraphEdge* origEdgep() ثابت {
        ثابت OrigEdgeList* ثابت oEListp = static_cast<OrigEdgeList*>(userp());
        UASSERT(oEListp, "No original edge associated with acyc edge " << این);
        بازگشت (oEListp->front());
    }

عمومی:
    GraphAcycEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* بالاترین, صحیح weight,
                  منطقی cutable = نادرست)
        : V3GraphEdge{graphp, fromp, بالاترین, weight, cutable} {}
    ~GraphAcycEdge() override = پیشفرض;
    // yellow=we might still cut it, else oldEdge: yellowGreen=made uncutable, red=uncutable
    string dotColor() ثابت override { بازگشت (cutable() ? "yellow" : origEdgep()->dotColor()); }
};

//--------------------------------------------------------------------

ساختار GraphAcycEdgeCmp final {
    منطقی operator()(ثابت V3GraphEdge* lhsp, ثابت V3GraphEdge* rhsp) ثابت {
        اگر (lhsp->weight() > rhsp->weight()) بازگشت درست;  // LHS goes first
        اگر (lhsp->weight() < rhsp->weight()) بازگشت نادرست;  // RHS goes first
        بازگشت نادرست;
    }
};

//--------------------------------------------------------------------

// CLASSES
رده GraphAcyc final {
    using OrigEdgeList
        = std::list<V3GraphEdge*>;  // List of orig edges, see also GraphAcycEdge's decl
    // GRAPH USERS
    //  origGraph
    //    GraphVertex::user()   GraphAycVerted* New graph node
    //  m_breakGraph
    //    GraphEdge::user()     OrigEdgeList*   Old graph edges
    //    GraphVertex::user     bool            Detection of loops in simplifyDupIterate
    // MEMBERS
    V3Graph* ثابت m_origGraphp;  // Original graph
    V3Graph m_breakGraph;  // Graph with only breakable edges represented
    GraphAcycVertex::List m_work;  // List of vertices with optimization work left
    std::vector<OrigEdgeList*> m_origEdgeDelp;  // List of deletions to do when done
    ثابت V3EdgeFuncP
        m_origEdgeFuncp;  // Function that says we follow this edge (in original graph)
    uint32_t m_placeStep = 0;  // Number that user() must be equal to to indicate processing

    // METHODS
    تهی buildGraph(V3Graph* origGraphp);
    تهی buildGraphIterate(V3GraphVertex* overtexp, GraphAcycVertex* avertexp);
    تهی simplify(منطقی allowCut);
    تهی simplifyNone(GraphAcycVertex* avertexp);
    تهی simplifyOne(GraphAcycVertex* avertexp);
    تهی simplifyOut(GraphAcycVertex* avertexp);
    تهی simplifyDup(GraphAcycVertex* avertexp);
    تهی cutBasic(GraphAcycVertex* avertexp);
    تهی cutBackward(GraphAcycVertex* avertexp);
    تهی deleteMarked();
    تهی place();
    تهی placeTryEdge(V3GraphEdge* edgep);
    منطقی placeIterate(GraphAcycVertex* vertexp, uint32_t currentRank);

    منطقی origFollowEdge(V3GraphEdge* edgep) {
        بازگشت (edgep->weight() && (m_origEdgeFuncp)(edgep));
    }
    تهی edgeFromEdge(V3GraphEdge* oldedgep, V3GraphVertex* fromp, V3GraphVertex* بالاترین) {
        // Make new breakGraph edge, with old edge as a template
        GraphAcycEdge* ثابت newEdgep = جدید GraphAcycEdge{&m_breakGraph, fromp, بالاترین,
                                                          oldedgep->weight(), oldedgep->cutable()};
        newEdgep->userp(oldedgep->userp());  // Keep pointer to OrigEdgeList
    }
    تهی addOrigEdgep(V3GraphEdge* toEdgep, V3GraphEdge* addEdgep) {
        // Add addEdge (or it's list) to list of edges that break edge represents
        // Note addEdge may already have a bunch of similar linked edge representations.  Yuk.
        UASSERT(addEdgep, "Adding nullptr");
        اگر (!toEdgep->userp()) {
            OrigEdgeList* ثابت oep = جدید OrigEdgeList;
            m_origEdgeDelp.افزودن_back(oep);
            toEdgep->userp(oep);
        }
        OrigEdgeList* ثابت oEListp = static_cast<OrigEdgeList*>(toEdgep->userp());
        اگر (OrigEdgeList* ثابت addListp = static_cast<OrigEdgeList*>(addEdgep->userp())) {
            برای (ثابت auto& itr : *addListp) oEListp->افزودن_back(itr);
            addListp->clear();  // Done with it
        } وگرنه {
            oEListp->افزودن_back(addEdgep);
        }
    }
    تهی cutOrigEdge(V3GraphEdge* breakEdgep, ثابت نویسه* why) {
        // From the break edge, cut edges in original graph it represents
        UINFO(8, why << " CUT " << breakEdgep->fromp());
        breakEdgep->cut();
        ثابت OrigEdgeList* ثابت oEListp = static_cast<OrigEdgeList*>(breakEdgep->userp());
        اگر (!oEListp) {
            v3fatalSrc("No original edge associated with cutting edge " << breakEdgep);
        }
        // The breakGraph edge may represent multiple real edges; cut them all
        برای (ثابت auto& origEdgep : *oEListp) {
            origEdgep->cut();
            UINFO(8, "  " << why << "   " << origEdgep->fromp() << " ->" << origEdgep->بالاترین());
        }
    }
    // Work Queue
    تهی work_افزودن(V3GraphVertex* vertexp) {
        GraphAcycVertex* ثابت avertexp = static_cast<GraphAcycVertex*>(vertexp);
        // Add vertex to list of nodes needing further optimization trials
        اگر (!avertexp->m_onWorkList) {
            avertexp->m_onWorkList = درست;
            m_work.linkBack(avertexp);
        }
    }
    GraphAcycVertex* workBeginp() { بازگشت m_work.frontp(); }
    تهی work_برداشت() {
        GraphAcycVertex* ثابت avertexp = workBeginp();
        avertexp->m_onWorkList = نادرست;
        m_work.unlink(avertexp);
    }

عمومی:
    // CONSTRUCTORS
    GraphAcyc(V3Graph* origGraphp, V3EdgeFuncP edgeFuncp)
        : m_origGraphp{origGraphp}
        , m_origEdgeFuncp{edgeFuncp} {}
    ~GraphAcyc() {
        برای (OrigEdgeList* ip : m_origEdgeDelp) delete ip;
        m_origEdgeDelp.clear();
    }
    تهی اصلی();
};

//--------------------------------------------------------------------

تهی GraphAcyc::buildGraph(V3Graph* origGraphp) {
    // Presumes the graph has been strongly ordered,
    // and thus there's a unique color if there are loops in this subgraph.

    // For each old node, make a new graph node for optimization
    origGraphp->userClearVertices();
    origGraphp->userClearEdges();
    برای (V3GraphVertex& overtex : origGraphp->vertices()) {
        اگر (overtex.color()) {
            GraphAcycVertex* ثابت avertexp = جدید GraphAcycVertex{&m_breakGraph, &overtex};
            overtex.userp(avertexp);  // Stash so can look up later
        }
    }

    // Build edges between logic vertices
    برای (V3GraphVertex& overtex : origGraphp->vertices()) {
        اگر (overtex.color()) {
            GraphAcycVertex* ثابت avertexp = static_cast<GraphAcycVertex*>(overtex.userp());
            buildGraphIterate(&overtex, avertexp);
        }
    }
}

تهی GraphAcyc::buildGraphIterate(V3GraphVertex* overtexp, GraphAcycVertex* avertexp) {
    // Make new edges
    برای (V3GraphEdge& edge : overtexp->outEdges()) {
        اگر (origFollowEdge(&edge)) {  // not cut
            ثابت V3GraphVertex* toVertexp = edge.بالاترین();
            اگر (toVertexp->color()) {
                GraphAcycVertex* ثابت toAVertexp
                    = static_cast<GraphAcycVertex*>(toVertexp->userp());
                // Replicate the old edge into the new graph
                // There may be multiple edges between same pairs of vertices
                V3GraphEdge* breakEdgep = جدید GraphAcycEdge{&m_breakGraph, avertexp, toAVertexp,
                                                            edge.weight(), edge.cutable()};
                addOrigEdgep(breakEdgep, &edge);  // So can find original edge
            }
        }
    }
}

تهی GraphAcyc::simplify(منطقی allowCut) {
    // Add all nodes to list of work to do
    برای (V3GraphVertex& vertex : m_breakGraph.vertices()) work_افزودن(&vertex);
    // Optimize till everything finished
    وقتی (GraphAcycVertex* vertexp = workBeginp()) {
        work_برداشت();
        simplifyNone(vertexp);
        simplifyOne(vertexp);
        simplifyOut(vertexp);
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

تهی GraphAcyc::deleteMarked() {
    // Delete nodes marked for removal
    برای (V3GraphVertex* ثابت vtxp : m_breakGraph.vertices().unlinkable()) {
        GraphAcycVertex* ثابت avertexp = static_cast<GraphAcycVertex*>(vtxp);
        اگر (avertexp->isDelete()) {
            VL_DO_DANGLING(avertexp->unlinkDelete(&m_breakGraph), avertexp);
        }
    }
}

تهی GraphAcyc::simplifyNone(GraphAcycVertex* avertexp) {
    // Don't need any vertices with no inputs, There's no way they can have a loop.
    // Likewise, vertices with no outputs
    اگر (avertexp->isDelete()) بازگشت;
    اگر (avertexp->inEmpty() || avertexp->outEmpty()) {
        UINFO(9, "  SimplifyNoneRemove " << avertexp);
        avertexp->setDelete();  // Mark so we won't delete it twice
        // Remove edges
        وقتی (V3GraphEdge* ثابت edgep = avertexp->outEdges().frontp()) {
            work_افزودن(edgep->بالاترین());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        وقتی (V3GraphEdge* ثابت edgep = avertexp->inEdges().frontp()) {
            work_افزودن(edgep->fromp());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
}

تهی GraphAcyc::simplifyOne(GraphAcycVertex* avertexp) {
    // If a node has one input and one output, we can remove it and change the edges
    اگر (avertexp->isDelete()) بازگشت;
    اگر (avertexp->inSize1() && avertexp->outSize1()) {
        V3GraphEdge* ثابت inEdgep = avertexp->inEdges().frontp();
        V3GraphEdge* ثابت outEdgep = avertexp->outEdges().frontp();
        V3GraphVertex* ثابت inVertexp = inEdgep->fromp();
        V3GraphVertex* ثابت outVertexp = outEdgep->بالاترین();
        // The in and out may be the same node; we'll make a loop
        // The in OR out may be THIS node; we can't delete it then.
        اگر (inVertexp != avertexp && outVertexp != avertexp) {
            UINFO(9, "  SimplifyOneRemove " << avertexp);
            avertexp->setDelete();  // Mark so we won't delete it twice
            // Make a new edge connecting the two vertices directly
            // If both are breakable, we pick the one with less weight, else it's arbitrary
            // We can forget about the origEdge list for the "non-selected" set of edges,
            // as we need to break only one set or the other set of edges, not both.
            // (This is why we must give preference to the cutable set.)
            V3GraphEdge* ثابت templateEdgep
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
            work_افزودن(inVertexp);
            work_افزودن(outVertexp);
        }
    }
}

تهی GraphAcyc::simplifyOut(GraphAcycVertex* avertexp) {
    // If a node has one output that's not cutable, all its inputs can be reassigned
    // to the next node in the list
    اگر (avertexp->isDelete()) بازگشت;
    اگر (avertexp->outSize1()) {
        V3GraphEdge* ثابت outEdgep = avertexp->outEdges().frontp();
        اگر (!outEdgep->cutable()) {
            V3GraphVertex* outVertexp = outEdgep->بالاترین();
            UINFO(9, "  SimplifyOutRemove " << avertexp);
            avertexp->setDelete();  // Mark so we won't delete it twice
            برای (V3GraphEdge* ثابت inEdgep : avertexp->inEdges().unlinkable()) {
                V3GraphVertex* inVertexp = inEdgep->fromp();
                اگر (inVertexp == avertexp) {
                    اگر (debug()) v3error("Non-cutable vertex=" << avertexp);  // LCOV_EXCL_LINE
                    v3error("Circular logic when ordering code (non-cutable edge loop)\n"
                            << m_origGraphp->reportLoops(  // calls OrderGraph::loopsVertexCb
                                   &V3GraphEdge::followNotCutable, avertexp->origVertexp()));
                    // Things are unlikely to end well at this point,
                    // but we'll try something to get to further errors...
                    inEdgep->cutable(درست);
                    بازگشت;
                }
                // Make a new edge connecting the two vertices directly
                // cppcheck-suppress leakReturnValNotUsed
                edgeFromEdge(inEdgep, inVertexp, outVertexp);
                // Remove old edge
                VL_DO_DANGLING(inEdgep->unlinkDelete(), inEdgep);
                work_افزودن(inVertexp);
            }
            VL_DO_DANGLING(outEdgep->unlinkDelete(), outEdgep);
            work_افزودن(outVertexp);
        }
    }
}

تهی GraphAcyc::simplifyDup(GraphAcycVertex* avertexp) {
    // Remove redundant edges
    اگر (avertexp->isDelete()) بازگشت;
    // Clear marks
    برای (V3GraphEdge& edge : avertexp->outEdges()) edge.بالاترین()->userp(nullptr);
    // Mark edges and detect duplications
    برای (V3GraphEdge* ثابت edgep : avertexp->outEdges().unlinkable()) {
        V3GraphVertex* outVertexp = edgep->بالاترین();
        V3GraphEdge* prevEdgep = static_cast<V3GraphEdge*>(outVertexp->userp());
        اگر (prevEdgep) {
            اگر (!prevEdgep->cutable()) {
                // !cutable duplicates prev !cutable: we can ignore it, redundant
                //  cutable duplicates prev !cutable: know it's not a relevant loop, ignore it
                UINFO(8, "    DelDupEdge " << avertexp << " -> " << edgep->بالاترین());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            } وگرنه اگر (!edgep->cutable()) {
                // !cutable duplicates prev  cutable: delete the earlier cutable
                UINFO(8, "    DelDupPrev " << avertexp << " -> " << prevEdgep->بالاترین());
                VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                outVertexp->userp(edgep);
            } وگرنه {
                //  cutable duplicates prev  cutable: combine weights
                UINFO(8, "    DelDupComb " << avertexp << " -> " << edgep->بالاترین());
                prevEdgep->weight(prevEdgep->weight() + edgep->weight());
                addOrigEdgep(prevEdgep, edgep);
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            work_افزودن(outVertexp);
            work_افزودن(avertexp);
        } وگرنه {
            // No previous assignment
            outVertexp->userp(edgep);
        }
    }
}

تهی GraphAcyc::cutBasic(GraphAcycVertex* avertexp) {
    // Detect and cleanup any loops from node to itself
    اگر (avertexp->isDelete()) بازگشت;
    برای (V3GraphEdge* ثابت edgep : avertexp->outEdges().unlinkable()) {
        اگر (edgep->cutable() && edgep->بالاترین() == avertexp) {
            cutOrigEdge(edgep, "  Cut Basic");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_افزودن(avertexp);
        }
    }
}

تهی GraphAcyc::cutBackward(GraphAcycVertex* avertexp) {
    // If a cutable edge is from A->B, and there's a non-cutable edge B->A, then must cut!
    اگر (avertexp->isDelete()) بازگشت;
    // Clear marks
    برای (V3GraphEdge& edge : avertexp->outEdges()) edge.بالاترین()->user(نادرست);
    برای (V3GraphEdge& edge : avertexp->inEdges()) {
        اگر (!edge.cutable()) edge.fromp()->user(درست);
    }
    // Detect duplications
    برای (V3GraphEdge* ثابت edgep : avertexp->outEdges().unlinkable()) {
        اگر (edgep->cutable() && edgep->بالاترین()->user()) {
            cutOrigEdge(edgep, "  Cut A->B->A");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_افزودن(avertexp);
        }
    }
}

تهی GraphAcyc::place() {
    // Input is m_breakGraph with ranks already assigned on non-breakable edges

    // Make a list of all cutable edges in the graph
    صحیح رقم_Edges = 0;
    برای (V3GraphVertex& vertex : m_breakGraph.vertices()) {
        برای (V3GraphEdge& edge : vertex.outEdges()) {
            اگر (edge.weight() && edge.cutable()) ++رقم_Edges;
        }
    }
    UINFO(4, "    Cutable edges = " << رقم_Edges);

    std::vector<V3GraphEdge*> edges;  // List of all edges to be processed
    // Make the vector properly sized right off the bat -- faster than reallocating
    edges.reserve(رقم_Edges + 1);
    برای (V3GraphVertex& vertex : m_breakGraph.vertices()) {
        vertex.user(0);  // Clear in prep of next step
        برای (V3GraphEdge& edge : vertex.outEdges()) {
            اگر (edge.weight() && edge.cutable()) edges.افزودن_back(&edge);
        }
    }

    // Sort by weight, then by vertex (so that we completely process one vertex, when possible)
    stable_sort(edges.begin(), edges.end(), GraphAcycEdgeCmp());

    // Process each edge in weighted order
    m_placeStep = 10;
    برای (V3GraphEdge* edgep : edges) placeTryEdge(edgep);
}

تهی GraphAcyc::placeTryEdge(V3GraphEdge* edgep) {
    // Try to make this edge uncutable
    m_placeStep++;
    UINFO(8, "    PlaceEdge s" << m_placeStep << " w" << edgep->weight() << " " << edgep->fromp());
    // Make the edge uncutable so we detect it in placement
    edgep->cutable(نادرست);
    // Vertex::m_user begin: number indicates this edge was completed
    // Try to assign ranks, presuming this edge is in place
    // If we come across user()==placestep, we've detected a loop and must back out
    ثابت منطقی loop
        = placeIterate(static_cast<GraphAcycVertex*>(edgep->بالاترین()), edgep->fromp()->rank() + 1);
    اگر (!loop) {
        // No loop, we can keep it as uncutable
        // Commit the new ranks we calculated
        // Just cleanup the list.  If this is slow, we can add another set of
        // user counters to avoid cleaning up the list.
        وقتی (workBeginp()) work_برداشت();
    } وگرنه {
        // Adding this edge would cause a loop, kill it
        edgep->cutable(درست);  // So graph still looks pretty
        cutOrigEdge(edgep, "  Cut loop");
        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        // Back out the ranks we calculated
        وقتی (GraphAcycVertex* vertexp = workBeginp()) {
            work_برداشت();
            vertexp->rank(vertexp->m_storedRank);
        }
    }
}

منطقی GraphAcyc::placeIterate(GraphAcycVertex* vertexp, uint32_t currentRank) {
    // Assign rank to each unvisited node
    //   rank() is the "committed rank" of the graph known without loops
    // If larger rank is found, assign it and loop back through
    // If we hit a back node make a list of all loops
    اگر (vertexp->rank() >= currentRank) بازگشت نادرست;  // Already processed it
    اگر (vertexp->user() == m_placeStep) بازگشت درست;  // Loop detected
    vertexp->user(m_placeStep);
    // Remember we're changing the rank of this node; might need to back out
    اگر (!vertexp->m_onWorkList) {
        vertexp->m_storedRank = vertexp->rank();
        work_افزودن(vertexp);
    }
    vertexp->rank(currentRank);
    // Follow all edges and increase their ranks
    برای (V3GraphEdge& edge : vertexp->outEdges()) {
        اگر (edge.weight() && !edge.cutable()) {
            اگر (placeIterate(static_cast<GraphAcycVertex*>(edge.بالاترین()), currentRank + 1)) {
                // We don't need to reset user(); we'll use a different placeStep for the next edge
                بازگشت درست;  // Loop detected
            }
        }
    }
    vertexp->user(0);
    بازگشت نادرست;
}

//----- Main algorithm entry point

تهی GraphAcyc::اصلی() {
    m_breakGraph.userClearEdges();

    // Color based on possible loops
    m_origGraphp->stronglyConnected(m_origEdgeFuncp);

    // Make a new graph with vertices that have only a single vertex
    // for each group of old vertices that are interconnected with unbreakable
    // edges (and thus can't represent loops - if we did the unbreakable
    // marking right, anyways)
    buildGraph(m_origGraphp);
    اگر (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_pre");

    // Perform simple optimizations before any cuttings
    simplify(نادرست);
    اگر (dumpGraphLevel() >= 5) m_breakGraph.dumpDotFilePrefixed("acyc_simp");

    UINFO(4, " Cutting trivial loops");
    simplify(درست);
    اگر (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_mid");

    UINFO(4, " Ranking");
    m_breakGraph.rank(&V3GraphEdge::followNotCutable);
    اگر (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_rank");

    UINFO(4, " Placement");
    place();
    اگر (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_place");

    UINFO(4, " Final Ranking");
    // Only needed to assert there are no loops in completed graph
    m_breakGraph.rank(&V3GraphEdge::followAlwaysTrue);
    اگر (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_done");
}

تهی V3Graph::acyclic(V3EdgeFuncP edgeFuncp) {
    UINFO(4, "Acyclic");
    GraphAcyc acyc{این, edgeFuncp};
    acyc.اصلی();
    UINFO(4, "Acyclic done");
}
