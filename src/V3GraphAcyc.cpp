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

صنف GraphAcycVertex final : عام V3GraphVertex {
    VL_RTTI_IMPL(GraphAcycVertex, V3GraphVertex)
    // user() is used for various sub-algorithm pieces
    V3GraphVertex* ثابت m_origVertexp;  // Pointer to first vertex this represents
محمي:
    friend صنف GraphAcyc;
    V3ListLinks<GraphAcycVertex> m_links;  // List links to store instances of this class
    uint32_t m_storedRank = 0;  // Rank held until commit to edge placement
    منطقي m_onWorkList = خطأ;  // True if already on list of work to do
    منطقي m_deleted = خطأ;  // True if deleted

خاص:
    V3ListLinks<GraphAcycVertex>& links() { أرجع m_links; }

عام:
    // List type to store instances of this class
    using List = V3List<GraphAcycVertex, &GraphAcycVertex::links>;

    GraphAcycVertex(V3Graph* graphp, V3GraphVertex* origVertexp)
        : V3GraphVertex{graphp}
        , m_origVertexp{origVertexp} {}
    ~GraphAcycVertex() override = افتراضي;
    V3GraphVertex* origVertexp() ثابت { أرجع m_origVertexp; }
    فراغ setDelete() { m_deleted = true; }
    منطقي isDelete() ثابت { أرجع m_deleted; }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string name() ثابت override { أرجع m_origVertexp->name(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string dotColor() ثابت override { أرجع m_origVertexp->dotColor(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    FileLine* fileline() ثابت override { أرجع m_origVertexp->fileline(); }
};

//--------------------------------------------------------------------

صنف GraphAcycEdge final : عام V3GraphEdge {
    VL_RTTI_IMPL(GraphAcycEdge, V3GraphEdge)
    // userp() is always used to point to the head original graph edge
خاص:
    using OrigEdgeList = std::list<V3GraphEdge*>;  // List of orig edges, see also GraphAcyc's decl
    V3GraphEdge* origEdgep() ثابت {
        ثابت OrigEdgeList* ثابت oEListp = static_cast<OrigEdgeList*>(userp());
        UASSERT(oEListp, "No original edge associated with acyc edge " << هذا);
        أرجع (oEListp->front());
    }

عام:
    GraphAcycEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* top, صحيح weight,
                  منطقي cutable = خطأ)
        : V3GraphEdge{graphp, fromp, top, weight, cutable} {}
    ~GraphAcycEdge() override = افتراضي;
    // yellow=we might still cut it, else oldEdge: yellowGreen=made uncutable, red=uncutable
    string dotColor() ثابت override { أرجع (cutable() ? "yellow" : origEdgep()->dotColor()); }
};

//--------------------------------------------------------------------

بنية GraphAcycEdgeCmp final {
    منطقي operator()(ثابت V3GraphEdge* lhsp, ثابت V3GraphEdge* rhsp) ثابت {
        𐤀𐤌 (lhsp->weight() > rhsp->weight()) أرجع true;  // LHS goes first
        𐤀𐤌 (lhsp->weight() < rhsp->weight()) أرجع خطأ;  // RHS goes first
        أرجع خطأ;
    }
};

//--------------------------------------------------------------------

// CLASSES
صنف GraphAcyc final {
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
    فراغ buildGraph(V3Graph* origGraphp);
    فراغ buildGraphIterate(V3GraphVertex* overtexp, GraphAcycVertex* avertexp);
    فراغ simplify(منطقي allowCut);
    فراغ simplifyNone(GraphAcycVertex* avertexp);
    فراغ simplifyOne(GraphAcycVertex* avertexp);
    فراغ simplifyOut(GraphAcycVertex* avertexp);
    فراغ simplifyDup(GraphAcycVertex* avertexp);
    فراغ cutBasic(GraphAcycVertex* avertexp);
    فراغ cutBackward(GraphAcycVertex* avertexp);
    فراغ deleteMarked();
    فراغ place();
    فراغ placeTryEdge(V3GraphEdge* edgep);
    منطقي placeIterate(GraphAcycVertex* vertexp, uint32_t currentRank);

    منطقي origFollowEdge(V3GraphEdge* edgep) {
        أرجع (edgep->weight() && (m_origEdgeFuncp)(edgep));
    }
    فراغ edgeFromEdge(V3GraphEdge* oldedgep, V3GraphVertex* fromp, V3GraphVertex* top) {
        // Make new breakGraph edge, with old edge as a template
        GraphAcycEdge* ثابت newEdgep = جديد GraphAcycEdge{&m_breakGraph, fromp, top,
                                                          oldedgep->weight(), oldedgep->cutable()};
        newEdgep->userp(oldedgep->userp());  // Keep pointer to OrigEdgeList
    }
    فراغ addOrigEdgep(V3GraphEdge* toEdgep, V3GraphEdge* addEdgep) {
        // Add addEdge (or it's list) to list of edges that break edge represents
        // Note addEdge may already have a bunch of similar linked edge representations.  Yuk.
        UASSERT(addEdgep, "Adding nullptr");
        𐤀𐤌 (!toEdgep->userp()) {
            OrigEdgeList* ثابت oep = جديد OrigEdgeList;
            m_origEdgeDelp.push_back(oep);
            toEdgep->userp(oep);
        }
        OrigEdgeList* ثابت oEListp = static_cast<OrigEdgeList*>(toEdgep->userp());
        𐤀𐤌 (OrigEdgeList* ثابت addListp = static_cast<OrigEdgeList*>(addEdgep->userp())) {
            لكل (ثابت auto& itr : *addListp) oEListp->push_back(itr);
            addListp->clear();  // Done with it
        } وإلا {
            oEListp->push_back(addEdgep);
        }
    }
    فراغ cutOrigEdge(V3GraphEdge* breakEdgep, ثابت حرف* why) {
        // From the break edge, cut edges in original graph it represents
        UINFO(8, why << " CUT " << breakEdgep->fromp());
        breakEdgep->cut();
        ثابت OrigEdgeList* ثابت oEListp = static_cast<OrigEdgeList*>(breakEdgep->userp());
        𐤀𐤌 (!oEListp) {
            v3fatalSrc("No original edge associated with cutting edge " << breakEdgep);
        }
        // The breakGraph edge may represent multiple real edges; cut them all
        لكل (ثابت auto& origEdgep : *oEListp) {
            origEdgep->cut();
            UINFO(8, "  " << why << "   " << origEdgep->fromp() << " ->" << origEdgep->top());
        }
    }
    // Work Queue
    فراغ workPush(V3GraphVertex* vertexp) {
        GraphAcycVertex* ثابت avertexp = static_cast<GraphAcycVertex*>(vertexp);
        // Add vertex to list of nodes needing further optimization trials
        𐤀𐤌 (!avertexp->m_onWorkList) {
            avertexp->m_onWorkList = true;
            m_work.linkBack(avertexp);
        }
    }
    GraphAcycVertex* workBeginp() { أرجع m_work.frontp(); }
    فراغ workPop() {
        GraphAcycVertex* ثابت avertexp = workBeginp();
        avertexp->m_onWorkList = خطأ;
        m_work.unlink(avertexp);
    }

عام:
    // CONSTRUCTORS
    GraphAcyc(V3Graph* origGraphp, V3EdgeFuncP edgeFuncp)
        : m_origGraphp{origGraphp}
        , m_origEdgeFuncp{edgeFuncp} {}
    ~GraphAcyc() {
        لكل (OrigEdgeList* ip : m_origEdgeDelp) delete ip;
        m_origEdgeDelp.clear();
    }
    فراغ main();
};

//--------------------------------------------------------------------

فراغ GraphAcyc::buildGraph(V3Graph* origGraphp) {
    // Presumes the graph has been strongly ordered,
    // and thus there's a unique color if there are loops in this subgraph.

    // For each old node, make a new graph node for optimization
    origGraphp->userClearVertices();
    origGraphp->userClearEdges();
    لكل (V3GraphVertex& overtex : origGraphp->vertices()) {
        𐤀𐤌 (overtex.color()) {
            GraphAcycVertex* ثابت avertexp = جديد GraphAcycVertex{&m_breakGraph, &overtex};
            overtex.userp(avertexp);  // Stash so can look up later
        }
    }

    // Build edges between logic vertices
    لكل (V3GraphVertex& overtex : origGraphp->vertices()) {
        𐤀𐤌 (overtex.color()) {
            GraphAcycVertex* ثابت avertexp = static_cast<GraphAcycVertex*>(overtex.userp());
            buildGraphIterate(&overtex, avertexp);
        }
    }
}

فراغ GraphAcyc::buildGraphIterate(V3GraphVertex* overtexp, GraphAcycVertex* avertexp) {
    // Make new edges
    لكل (V3GraphEdge& edge : overtexp->outEdges()) {
        𐤀𐤌 (origFollowEdge(&edge)) {  // not cut
            ثابت V3GraphVertex* toVertexp = edge.top();
            𐤀𐤌 (toVertexp->color()) {
                GraphAcycVertex* ثابت toAVertexp
                    = static_cast<GraphAcycVertex*>(toVertexp->userp());
                // Replicate the old edge into the new graph
                // There may be multiple edges between same pairs of vertices
                V3GraphEdge* breakEdgep = جديد GraphAcycEdge{&m_breakGraph, avertexp, toAVertexp,
                                                            edge.weight(), edge.cutable()};
                addOrigEdgep(breakEdgep, &edge);  // So can find original edge
            }
        }
    }
}

فراغ GraphAcyc::simplify(منطقي allowCut) {
    // Add all nodes to list of work to do
    لكل (V3GraphVertex& vertex : m_breakGraph.vertices()) workPush(&vertex);
    // Optimize till everything finished
    طالما (GraphAcycVertex* vertexp = workBeginp()) {
        workPop();
        simplifyNone(vertexp);
        simplifyOne(vertexp);
        simplifyOut(vertexp);
        simplifyDup(vertexp);
        𐤀𐤌 (allowCut) {
            // The main algorithm works without these, though slower
            // So if changing the main algorithm, comment these out for a test run
            𐤀𐤌 (v3Global.opt.fAcycSimp()) {
                cutBasic(vertexp);
                cutBackward(vertexp);
            }
        }
    }
    deleteMarked();
}

فراغ GraphAcyc::deleteMarked() {
    // Delete nodes marked for removal
    لكل (V3GraphVertex* ثابت vtxp : m_breakGraph.vertices().unlinkable()) {
        GraphAcycVertex* ثابت avertexp = static_cast<GraphAcycVertex*>(vtxp);
        𐤀𐤌 (avertexp->isDelete()) {
            VL_DO_DANGLING(avertexp->unlinkDelete(&m_breakGraph), avertexp);
        }
    }
}

فراغ GraphAcyc::simplifyNone(GraphAcycVertex* avertexp) {
    // Don't need any vertices with no inputs, There's no way they can have a loop.
    // Likewise, vertices with no outputs
    𐤀𐤌 (avertexp->isDelete()) أرجع;
    𐤀𐤌 (avertexp->inEmpty() || avertexp->outEmpty()) {
        UINFO(9, "  SimplifyNoneRemove " << avertexp);
        avertexp->setDelete();  // Mark so we won't delete it twice
        // Remove edges
        طالما (V3GraphEdge* ثابت edgep = avertexp->outEdges().frontp()) {
            workPush(edgep->top());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        طالما (V3GraphEdge* ثابت edgep = avertexp->inEdges().frontp()) {
            workPush(edgep->fromp());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
}

فراغ GraphAcyc::simplifyOne(GraphAcycVertex* avertexp) {
    // If a node has one input and one output, we can remove it and change the edges
    𐤀𐤌 (avertexp->isDelete()) أرجع;
    𐤀𐤌 (avertexp->inSize1() && avertexp->outSize1()) {
        V3GraphEdge* ثابت inEdgep = avertexp->inEdges().frontp();
        V3GraphEdge* ثابت outEdgep = avertexp->outEdges().frontp();
        V3GraphVertex* ثابت inVertexp = inEdgep->fromp();
        V3GraphVertex* ثابت outVertexp = outEdgep->top();
        // The in and out may be the same node; we'll make a loop
        // The in OR out may be THIS node; we can't delete it then.
        𐤀𐤌 (inVertexp != avertexp && outVertexp != avertexp) {
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
            workPush(inVertexp);
            workPush(outVertexp);
        }
    }
}

فراغ GraphAcyc::simplifyOut(GraphAcycVertex* avertexp) {
    // If a node has one output that's not cutable, all its inputs can be reassigned
    // to the next node in the list
    𐤀𐤌 (avertexp->isDelete()) أرجع;
    𐤀𐤌 (avertexp->outSize1()) {
        V3GraphEdge* ثابت outEdgep = avertexp->outEdges().frontp();
        𐤀𐤌 (!outEdgep->cutable()) {
            V3GraphVertex* outVertexp = outEdgep->top();
            UINFO(9, "  SimplifyOutRemove " << avertexp);
            avertexp->setDelete();  // Mark so we won't delete it twice
            لكل (V3GraphEdge* ثابت inEdgep : avertexp->inEdges().unlinkable()) {
                V3GraphVertex* inVertexp = inEdgep->fromp();
                𐤀𐤌 (inVertexp == avertexp) {
                    𐤀𐤌 (debug()) v3error("Non-cutable vertex=" << avertexp);  // LCOV_EXCL_LINE
                    v3error("Circular logic when ordering code (non-cutable edge loop)\n"
                            << m_origGraphp->reportLoops(  // calls OrderGraph::loopsVertexCb
                                   &V3GraphEdge::followNotCutable, avertexp->origVertexp()));
                    // Things are unlikely to end well at this point,
                    // but we'll try something to get to further errors...
                    inEdgep->cutable(true);
                    أرجع;
                }
                // Make a new edge connecting the two vertices directly
                // cppcheck-suppress leakReturnValNotUsed
                edgeFromEdge(inEdgep, inVertexp, outVertexp);
                // Remove old edge
                VL_DO_DANGLING(inEdgep->unlinkDelete(), inEdgep);
                workPush(inVertexp);
            }
            VL_DO_DANGLING(outEdgep->unlinkDelete(), outEdgep);
            workPush(outVertexp);
        }
    }
}

فراغ GraphAcyc::simplifyDup(GraphAcycVertex* avertexp) {
    // Remove redundant edges
    𐤀𐤌 (avertexp->isDelete()) أرجع;
    // Clear marks
    لكل (V3GraphEdge& edge : avertexp->outEdges()) edge.top()->userp(nullptr);
    // Mark edges and detect duplications
    لكل (V3GraphEdge* ثابت edgep : avertexp->outEdges().unlinkable()) {
        V3GraphVertex* outVertexp = edgep->top();
        V3GraphEdge* prevEdgep = static_cast<V3GraphEdge*>(outVertexp->userp());
        𐤀𐤌 (prevEdgep) {
            𐤀𐤌 (!prevEdgep->cutable()) {
                // !cutable duplicates prev !cutable: we can ignore it, redundant
                //  cutable duplicates prev !cutable: know it's not a relevant loop, ignore it
                UINFO(8, "    DelDupEdge " << avertexp << " -> " << edgep->top());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            } وإلا 𐤀𐤌 (!edgep->cutable()) {
                // !cutable duplicates prev  cutable: delete the earlier cutable
                UINFO(8, "    DelDupPrev " << avertexp << " -> " << prevEdgep->top());
                VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                outVertexp->userp(edgep);
            } وإلا {
                //  cutable duplicates prev  cutable: combine weights
                UINFO(8, "    DelDupComb " << avertexp << " -> " << edgep->top());
                prevEdgep->weight(prevEdgep->weight() + edgep->weight());
                addOrigEdgep(prevEdgep, edgep);
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            workPush(outVertexp);
            workPush(avertexp);
        } وإلا {
            // No previous assignment
            outVertexp->userp(edgep);
        }
    }
}

فراغ GraphAcyc::cutBasic(GraphAcycVertex* avertexp) {
    // Detect and cleanup any loops from node to itself
    𐤀𐤌 (avertexp->isDelete()) أرجع;
    لكل (V3GraphEdge* ثابت edgep : avertexp->outEdges().unlinkable()) {
        𐤀𐤌 (edgep->cutable() && edgep->top() == avertexp) {
            cutOrigEdge(edgep, "  Cut Basic");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            workPush(avertexp);
        }
    }
}

فراغ GraphAcyc::cutBackward(GraphAcycVertex* avertexp) {
    // If a cutable edge is from A->B, and there's a non-cutable edge B->A, then must cut!
    𐤀𐤌 (avertexp->isDelete()) أرجع;
    // Clear marks
    لكل (V3GraphEdge& edge : avertexp->outEdges()) edge.top()->user(خطأ);
    لكل (V3GraphEdge& edge : avertexp->inEdges()) {
        𐤀𐤌 (!edge.cutable()) edge.fromp()->user(true);
    }
    // Detect duplications
    لكل (V3GraphEdge* ثابت edgep : avertexp->outEdges().unlinkable()) {
        𐤀𐤌 (edgep->cutable() && edgep->top()->user()) {
            cutOrigEdge(edgep, "  Cut A->B->A");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            workPush(avertexp);
        }
    }
}

فراغ GraphAcyc::place() {
    // Input is m_breakGraph with ranks already assigned on non-breakable edges

    // Make a list of all cutable edges in the graph
    صحيح numEdges = 0;
    لكل (V3GraphVertex& vertex : m_breakGraph.vertices()) {
        لكل (V3GraphEdge& edge : vertex.outEdges()) {
            𐤀𐤌 (edge.weight() && edge.cutable()) ++numEdges;
        }
    }
    UINFO(4, "    Cutable edges = " << numEdges);

    std::vector<V3GraphEdge*> edges;  // List of all edges to be processed
    // Make the vector properly sized right off the bat -- faster than reallocating
    edges.reserve(numEdges + 1);
    لكل (V3GraphVertex& vertex : m_breakGraph.vertices()) {
        vertex.user(0);  // Clear in prep of next step
        لكل (V3GraphEdge& edge : vertex.outEdges()) {
            𐤀𐤌 (edge.weight() && edge.cutable()) edges.push_back(&edge);
        }
    }

    // Sort by weight, then by vertex (so that we completely process one vertex, when possible)
    stable_sort(edges.begin(), edges.end(), GraphAcycEdgeCmp());

    // Process each edge in weighted order
    m_placeStep = 10;
    لكل (V3GraphEdge* edgep : edges) placeTryEdge(edgep);
}

فراغ GraphAcyc::placeTryEdge(V3GraphEdge* edgep) {
    // Try to make this edge uncutable
    m_placeStep++;
    UINFO(8, "    PlaceEdge s" << m_placeStep << " w" << edgep->weight() << " " << edgep->fromp());
    // Make the edge uncutable so we detect it in placement
    edgep->cutable(خطأ);
    // Vertex::m_user begin: number indicates this edge was completed
    // Try to assign ranks, presuming this edge is in place
    // If we come across user()==placestep, we've detected a loop and must back out
    ثابت منطقي loop
        = placeIterate(static_cast<GraphAcycVertex*>(edgep->top()), edgep->fromp()->rank() + 1);
    𐤀𐤌 (!loop) {
        // No loop, we can keep it as uncutable
        // Commit the new ranks we calculated
        // Just cleanup the list.  If this is slow, we can add another set of
        // user counters to avoid cleaning up the list.
        طالما (workBeginp()) workPop();
    } وإلا {
        // Adding this edge would cause a loop, kill it
        edgep->cutable(true);  // So graph still looks pretty
        cutOrigEdge(edgep, "  Cut loop");
        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        // Back out the ranks we calculated
        طالما (GraphAcycVertex* vertexp = workBeginp()) {
            workPop();
            vertexp->rank(vertexp->m_storedRank);
        }
    }
}

منطقي GraphAcyc::placeIterate(GraphAcycVertex* vertexp, uint32_t currentRank) {
    // Assign rank to each unvisited node
    //   rank() is the "committed rank" of the graph known without loops
    // If larger rank is found, assign it and loop back through
    // If we hit a back node make a list of all loops
    𐤀𐤌 (vertexp->rank() >= currentRank) أرجع خطأ;  // Already processed it
    𐤀𐤌 (vertexp->user() == m_placeStep) أرجع true;  // Loop detected
    vertexp->user(m_placeStep);
    // Remember we're changing the rank of this node; might need to back out
    𐤀𐤌 (!vertexp->m_onWorkList) {
        vertexp->m_storedRank = vertexp->rank();
        workPush(vertexp);
    }
    vertexp->rank(currentRank);
    // Follow all edges and increase their ranks
    لكل (V3GraphEdge& edge : vertexp->outEdges()) {
        𐤀𐤌 (edge.weight() && !edge.cutable()) {
            𐤀𐤌 (placeIterate(static_cast<GraphAcycVertex*>(edge.top()), currentRank + 1)) {
                // We don't need to reset user(); we'll use a different placeStep for the next edge
                أرجع true;  // Loop detected
            }
        }
    }
    vertexp->user(0);
    أرجع خطأ;
}

//----- Main algorithm entry point

فراغ GraphAcyc::main() {
    m_breakGraph.userClearEdges();

    // Color based on possible loops
    m_origGraphp->stronglyConnected(m_origEdgeFuncp);

    // Make a new graph with vertices that have only a single vertex
    // for each group of old vertices that are interconnected with unbreakable
    // edges (and thus can't represent loops - if we did the unbreakable
    // marking right, anyways)
    buildGraph(m_origGraphp);
    𐤀𐤌 (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_pre");

    // Perform simple optimizations before any cuttings
    simplify(خطأ);
    𐤀𐤌 (dumpGraphLevel() >= 5) m_breakGraph.dumpDotFilePrefixed("acyc_simp");

    UINFO(4, " Cutting trivial loops");
    simplify(true);
    𐤀𐤌 (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_mid");

    UINFO(4, " Ranking");
    m_breakGraph.rank(&V3GraphEdge::followNotCutable);
    𐤀𐤌 (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_rank");

    UINFO(4, " Placement");
    place();
    𐤀𐤌 (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_place");

    UINFO(4, " Final Ranking");
    // Only needed to assert there are no loops in completed graph
    m_breakGraph.rank(&V3GraphEdge::followAlwaysTrue);
    𐤀𐤌 (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_done");
}

فراغ V3Graph::acyclic(V3EdgeFuncP edgeFuncp) {
    UINFO(4, "Acyclic");
    GraphAcyc acyc{هذا, edgeFuncp};
    acyc.main();
    UINFO(4, "Acyclic done");
}
