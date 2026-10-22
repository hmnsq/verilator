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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3Graph.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <list>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - acyclic
//      Break the minimal number of backward edges to make the graph acyclic

വർഗ്ഗം GraphAcycVertex final : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(GraphAcycVertex, V3GraphVertex)
    // user() is used for various sub-algorithm pieces
    V3GraphVertex* സ്ഥിരം m_origVertexp;  // Pointer to first vertex this represents
സംരക്ഷിത:
    friend വർഗ്ഗം GraphAcyc;
    V3ListLinks<GraphAcycVertex> m_links;  // List links to store instances of this class
    uint32_t m_storedRank = 0;  // Rank held until commit to edge placement
    തർക്കം m_onWorkList = അസത്യം;  // True if already on list of work to do
    തർക്കം m_deleted = അസത്യം;  // True if deleted

സ്വകാര്യ:
    V3ListLinks<GraphAcycVertex>& links() { തിരികെ m_links; }

പൊതു:
    // List type to store instances of this class
    using List = V3List<GraphAcycVertex, &GraphAcycVertex::links>;

    GraphAcycVertex(V3Graph* graphp, V3GraphVertex* origVertexp)
        : V3GraphVertex{graphp}
        , m_origVertexp{origVertexp} {}
    ~GraphAcycVertex() override = സ്ഥിരസ്ഥിതി;
    V3GraphVertex* origVertexp() സ്ഥിരം { തിരികെ m_origVertexp; }
    ശൂന്യം setDelete() { m_deleted = സത്യം; }
    തർക്കം isDelete() സ്ഥിരം { തിരികെ m_deleted; }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string name() സ്ഥിരം override { തിരികെ m_origVertexp->name(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string dotColor() സ്ഥിരം override { തിരികെ m_origVertexp->dotColor(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    File_വരി* fileline() സ്ഥിരം override { തിരികെ m_origVertexp->fileline(); }
};

//--------------------------------------------------------------------

വർഗ്ഗം GraphAcycEdge final : പൊതു V3GraphEdge {
    VL_RTTI_IMPL(GraphAcycEdge, V3GraphEdge)
    // userp() is always used to point to the head original graph edge
സ്വകാര്യ:
    using OrigEdgeList = std::list<V3GraphEdge*>;  // List of orig edges, see also GraphAcyc's decl
    V3GraphEdge* origEdgep() സ്ഥിരം {
        സ്ഥിരം OrigEdgeList* സ്ഥിരം oEListp = static_cast<OrigEdgeList*>(userp());
        UASSERT(oEListp, "No original edge associated with acyc edge " << ഇത്);
        തിരികെ (oEListp->front());
    }

പൊതു:
    GraphAcycEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* മുകൾ, പൂർണ്ണസംഖ്യ weight,
                  തർക്കം cutable = അസത്യം)
        : V3GraphEdge{graphp, fromp, മുകൾ, weight, cutable} {}
    ~GraphAcycEdge() override = സ്ഥിരസ്ഥിതി;
    // yellow=we might still cut it, else oldEdge: yellowGreen=made uncutable, red=uncutable
    string dotColor() സ്ഥിരം override { തിരികെ (cutable() ? "yellow" : origEdgep()->dotColor()); }
};

//--------------------------------------------------------------------

ഘടന GraphAcycEdgeCmp final {
    തർക്കം operator()(സ്ഥിരം V3GraphEdge* lhsp, സ്ഥിരം V3GraphEdge* rhsp) സ്ഥിരം {
        എങ്കിൽ (lhsp->weight() > rhsp->weight()) തിരികെ സത്യം;  // LHS goes first
        എങ്കിൽ (lhsp->weight() < rhsp->weight()) തിരികെ അസത്യം;  // RHS goes first
        തിരികെ അസത്യം;
    }
};

//--------------------------------------------------------------------

// CLASSES
വർഗ്ഗം GraphAcyc final {
    using OrigEdgeList
        = std::list<V3GraphEdge*>;  // List of orig edges, see also GraphAcycEdge's decl
    // GRAPH USERS
    //  origGraph
    //    GraphVertex::user()   GraphAycVerted* New graph node
    //  m_breakGraph
    //    GraphEdge::user()     OrigEdgeList*   Old graph edges
    //    GraphVertex::user     bool            Detection of loops in simplifyDupIterate
    // MEMBERS
    V3Graph* സ്ഥിരം m_origGraphp;  // Original graph
    V3Graph m_breakGraph;  // Graph with only breakable edges represented
    GraphAcycVertex::List m_work;  // List of vertices with optimization work left
    std::vector<OrigEdgeList*> m_origEdgeDelp;  // List of deletions to do when done
    സ്ഥിരം V3EdgeFuncP
        m_origEdgeFuncp;  // Function that says we follow this edge (in original graph)
    uint32_t m_placeStep = 0;  // Number that user() must be equal to to indicate processing

    // METHODS
    ശൂന്യം buildGraph(V3Graph* origGraphp);
    ശൂന്യം buildGraphIterate(V3GraphVertex* overtexp, GraphAcycVertex* avertexp);
    ശൂന്യം simplify(തർക്കം allowCut);
    ശൂന്യം simplifyNone(GraphAcycVertex* avertexp);
    ശൂന്യം simplifyOne(GraphAcycVertex* avertexp);
    ശൂന്യം simplifyOut(GraphAcycVertex* avertexp);
    ശൂന്യം simplifyDup(GraphAcycVertex* avertexp);
    ശൂന്യം cutBasic(GraphAcycVertex* avertexp);
    ശൂന്യം cutBackward(GraphAcycVertex* avertexp);
    ശൂന്യം deleteMarked();
    ശൂന്യം place();
    ശൂന്യം placeTryEdge(V3GraphEdge* edgep);
    തർക്കം placeIterate(GraphAcycVertex* vertexp, uint32_t currentRank);

    തർക്കം origFollowEdge(V3GraphEdge* edgep) {
        തിരികെ (edgep->weight() && (m_origEdgeFuncp)(edgep));
    }
    ശൂന്യം edgeFromEdge(V3GraphEdge* oldedgep, V3GraphVertex* fromp, V3GraphVertex* മുകൾ) {
        // Make new breakGraph edge, with old edge as a template
        GraphAcycEdge* സ്ഥിരം newEdgep = പുതിയ GraphAcycEdge{&m_breakGraph, fromp, മുകൾ,
                                                          oldedgep->weight(), oldedgep->cutable()};
        newEdgep->userp(oldedgep->userp());  // Keep pointer to OrigEdgeList
    }
    ശൂന്യം addOrigEdgep(V3GraphEdge* toEdgep, V3GraphEdge* addEdgep) {
        // Add addEdge (or it's list) to list of edges that break edge represents
        // Note addEdge may already have a bunch of similar linked edge representations.  Yuk.
        UASSERT(addEdgep, "Adding nullptr");
        എങ്കിൽ (!toEdgep->userp()) {
            OrigEdgeList* സ്ഥിരം oep = പുതിയ OrigEdgeList;
            m_origEdgeDelp.തള്ളുക_back(oep);
            toEdgep->userp(oep);
        }
        OrigEdgeList* സ്ഥിരം oEListp = static_cast<OrigEdgeList*>(toEdgep->userp());
        എങ്കിൽ (OrigEdgeList* സ്ഥിരം addListp = static_cast<OrigEdgeList*>(addEdgep->userp())) {
            വേണ്ടി (സ്ഥിരം auto& itr : *addListp) oEListp->തള്ളുക_back(itr);
            addListp->clear();  // Done with it
        } അല്ലെങ്കിൽ {
            oEListp->തള്ളുക_back(addEdgep);
        }
    }
    ശൂന്യം cutOrigEdge(V3GraphEdge* breakEdgep, സ്ഥിരം അക്ഷരം* why) {
        // From the break edge, cut edges in original graph it represents
        UINFO(8, why << " CUT " << breakEdgep->fromp());
        breakEdgep->cut();
        സ്ഥിരം OrigEdgeList* സ്ഥിരം oEListp = static_cast<OrigEdgeList*>(breakEdgep->userp());
        എങ്കിൽ (!oEListp) {
            v3fatalSrc("No original edge associated with cutting edge " << breakEdgep);
        }
        // The breakGraph edge may represent multiple real edges; cut them all
        വേണ്ടി (സ്ഥിരം auto& origEdgep : *oEListp) {
            origEdgep->cut();
            UINFO(8, "  " << why << "   " << origEdgep->fromp() << " ->" << origEdgep->മുകൾ());
        }
    }
    // Work Queue
    ശൂന്യം work_തള്ളുക(V3GraphVertex* vertexp) {
        GraphAcycVertex* സ്ഥിരം avertexp = static_cast<GraphAcycVertex*>(vertexp);
        // Add vertex to list of nodes needing further optimization trials
        എങ്കിൽ (!avertexp->m_onWorkList) {
            avertexp->m_onWorkList = സത്യം;
            m_work.linkBack(avertexp);
        }
    }
    GraphAcycVertex* workBeginp() { തിരികെ m_work.frontp(); }
    ശൂന്യം work_എടുക്കുക() {
        GraphAcycVertex* സ്ഥിരം avertexp = workBeginp();
        avertexp->m_onWorkList = അസത്യം;
        m_work.unlink(avertexp);
    }

പൊതു:
    // CONSTRUCTORS
    GraphAcyc(V3Graph* origGraphp, V3EdgeFuncP edgeFuncp)
        : m_origGraphp{origGraphp}
        , m_origEdgeFuncp{edgeFuncp} {}
    ~GraphAcyc() {
        വേണ്ടി (OrigEdgeList* ip : m_origEdgeDelp) delete ip;
        m_origEdgeDelp.clear();
    }
    ശൂന്യം പ്രധാന();
};

//--------------------------------------------------------------------

ശൂന്യം GraphAcyc::buildGraph(V3Graph* origGraphp) {
    // Presumes the graph has been strongly ordered,
    // and thus there's a unique color if there are loops in this subgraph.

    // For each old node, make a new graph node for optimization
    origGraphp->userClearVertices();
    origGraphp->userClearEdges();
    വേണ്ടി (V3GraphVertex& overtex : origGraphp->vertices()) {
        എങ്കിൽ (overtex.color()) {
            GraphAcycVertex* സ്ഥിരം avertexp = പുതിയ GraphAcycVertex{&m_breakGraph, &overtex};
            overtex.userp(avertexp);  // Stash so can look up later
        }
    }

    // Build edges between logic vertices
    വേണ്ടി (V3GraphVertex& overtex : origGraphp->vertices()) {
        എങ്കിൽ (overtex.color()) {
            GraphAcycVertex* സ്ഥിരം avertexp = static_cast<GraphAcycVertex*>(overtex.userp());
            buildGraphIterate(&overtex, avertexp);
        }
    }
}

ശൂന്യം GraphAcyc::buildGraphIterate(V3GraphVertex* overtexp, GraphAcycVertex* avertexp) {
    // Make new edges
    വേണ്ടി (V3GraphEdge& edge : overtexp->outEdges()) {
        എങ്കിൽ (origFollowEdge(&edge)) {  // not cut
            സ്ഥിരം V3GraphVertex* toVertexp = edge.മുകൾ();
            എങ്കിൽ (toVertexp->color()) {
                GraphAcycVertex* സ്ഥിരം toAVertexp
                    = static_cast<GraphAcycVertex*>(toVertexp->userp());
                // Replicate the old edge into the new graph
                // There may be multiple edges between same pairs of vertices
                V3GraphEdge* breakEdgep = പുതിയ GraphAcycEdge{&m_breakGraph, avertexp, toAVertexp,
                                                            edge.weight(), edge.cutable()};
                addOrigEdgep(breakEdgep, &edge);  // So can find original edge
            }
        }
    }
}

ശൂന്യം GraphAcyc::simplify(തർക്കം allowCut) {
    // Add all nodes to list of work to do
    വേണ്ടി (V3GraphVertex& vertex : m_breakGraph.vertices()) work_തള്ളുക(&vertex);
    // Optimize till everything finished
    ആകുമ്പോൾ (GraphAcycVertex* vertexp = workBeginp()) {
        work_എടുക്കുക();
        simplifyNone(vertexp);
        simplifyOne(vertexp);
        simplifyOut(vertexp);
        simplifyDup(vertexp);
        എങ്കിൽ (allowCut) {
            // The main algorithm works without these, though slower
            // So if changing the main algorithm, comment these out for a test run
            എങ്കിൽ (v3Global.opt.fAcycSimp()) {
                cutBasic(vertexp);
                cutBackward(vertexp);
            }
        }
    }
    deleteMarked();
}

ശൂന്യം GraphAcyc::deleteMarked() {
    // Delete nodes marked for removal
    വേണ്ടി (V3GraphVertex* സ്ഥിരം vtxp : m_breakGraph.vertices().unlinkable()) {
        GraphAcycVertex* സ്ഥിരം avertexp = static_cast<GraphAcycVertex*>(vtxp);
        എങ്കിൽ (avertexp->isDelete()) {
            VL_DO_DANGLING(avertexp->unlinkDelete(&m_breakGraph), avertexp);
        }
    }
}

ശൂന്യം GraphAcyc::simplifyNone(GraphAcycVertex* avertexp) {
    // Don't need any vertices with no inputs, There's no way they can have a loop.
    // Likewise, vertices with no outputs
    എങ്കിൽ (avertexp->isDelete()) തിരികെ;
    എങ്കിൽ (avertexp->inEmpty() || avertexp->outEmpty()) {
        UINFO(9, "  SimplifyNoneRemove " << avertexp);
        avertexp->setDelete();  // Mark so we won't delete it twice
        // Remove edges
        ആകുമ്പോൾ (V3GraphEdge* സ്ഥിരം edgep = avertexp->outEdges().frontp()) {
            work_തള്ളുക(edgep->മുകൾ());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        ആകുമ്പോൾ (V3GraphEdge* സ്ഥിരം edgep = avertexp->inEdges().frontp()) {
            work_തള്ളുക(edgep->fromp());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
}

ശൂന്യം GraphAcyc::simplifyOne(GraphAcycVertex* avertexp) {
    // If a node has one input and one output, we can remove it and change the edges
    എങ്കിൽ (avertexp->isDelete()) തിരികെ;
    എങ്കിൽ (avertexp->inSize1() && avertexp->outSize1()) {
        V3GraphEdge* സ്ഥിരം inEdgep = avertexp->inEdges().frontp();
        V3GraphEdge* സ്ഥിരം outEdgep = avertexp->outEdges().frontp();
        V3GraphVertex* സ്ഥിരം inVertexp = inEdgep->fromp();
        V3GraphVertex* സ്ഥിരം outVertexp = outEdgep->മുകൾ();
        // The in and out may be the same node; we'll make a loop
        // The in OR out may be THIS node; we can't delete it then.
        എങ്കിൽ (inVertexp != avertexp && outVertexp != avertexp) {
            UINFO(9, "  SimplifyOneRemove " << avertexp);
            avertexp->setDelete();  // Mark so we won't delete it twice
            // Make a new edge connecting the two vertices directly
            // If both are breakable, we pick the one with less weight, else it's arbitrary
            // We can forget about the origEdge list for the "non-selected" set of edges,
            // as we need to break only one set or the other set of edges, not both.
            // (This is why we must give preference to the cutable set.)
            V3GraphEdge* സ്ഥിരം templateEdgep
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
            work_തള്ളുക(inVertexp);
            work_തള്ളുക(outVertexp);
        }
    }
}

ശൂന്യം GraphAcyc::simplifyOut(GraphAcycVertex* avertexp) {
    // If a node has one output that's not cutable, all its inputs can be reassigned
    // to the next node in the list
    എങ്കിൽ (avertexp->isDelete()) തിരികെ;
    എങ്കിൽ (avertexp->outSize1()) {
        V3GraphEdge* സ്ഥിരം outEdgep = avertexp->outEdges().frontp();
        എങ്കിൽ (!outEdgep->cutable()) {
            V3GraphVertex* outVertexp = outEdgep->മുകൾ();
            UINFO(9, "  SimplifyOutRemove " << avertexp);
            avertexp->setDelete();  // Mark so we won't delete it twice
            വേണ്ടി (V3GraphEdge* സ്ഥിരം inEdgep : avertexp->inEdges().unlinkable()) {
                V3GraphVertex* inVertexp = inEdgep->fromp();
                എങ്കിൽ (inVertexp == avertexp) {
                    എങ്കിൽ (debug()) v3error("Non-cutable vertex=" << avertexp);  // LCOV_EXCL_LINE
                    v3error("Circular logic when ordering code (non-cutable edge loop)\n"
                            << m_origGraphp->reportLoops(  // calls OrderGraph::loopsVertexCb
                                   &V3GraphEdge::followNotCutable, avertexp->origVertexp()));
                    // Things are unlikely to end well at this point,
                    // but we'll try something to get to further errors...
                    inEdgep->cutable(സത്യം);
                    തിരികെ;
                }
                // Make a new edge connecting the two vertices directly
                // cppcheck-suppress leakReturnValNotUsed
                edgeFromEdge(inEdgep, inVertexp, outVertexp);
                // Remove old edge
                VL_DO_DANGLING(inEdgep->unlinkDelete(), inEdgep);
                work_തള്ളുക(inVertexp);
            }
            VL_DO_DANGLING(outEdgep->unlinkDelete(), outEdgep);
            work_തള്ളുക(outVertexp);
        }
    }
}

ശൂന്യം GraphAcyc::simplifyDup(GraphAcycVertex* avertexp) {
    // Remove redundant edges
    എങ്കിൽ (avertexp->isDelete()) തിരികെ;
    // Clear marks
    വേണ്ടി (V3GraphEdge& edge : avertexp->outEdges()) edge.മുകൾ()->userp(nullptr);
    // Mark edges and detect duplications
    വേണ്ടി (V3GraphEdge* സ്ഥിരം edgep : avertexp->outEdges().unlinkable()) {
        V3GraphVertex* outVertexp = edgep->മുകൾ();
        V3GraphEdge* prevEdgep = static_cast<V3GraphEdge*>(outVertexp->userp());
        എങ്കിൽ (prevEdgep) {
            എങ്കിൽ (!prevEdgep->cutable()) {
                // !cutable duplicates prev !cutable: we can ignore it, redundant
                //  cutable duplicates prev !cutable: know it's not a relevant loop, ignore it
                UINFO(8, "    DelDupEdge " << avertexp << " -> " << edgep->മുകൾ());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            } അല്ലെങ്കിൽ എങ്കിൽ (!edgep->cutable()) {
                // !cutable duplicates prev  cutable: delete the earlier cutable
                UINFO(8, "    DelDupPrev " << avertexp << " -> " << prevEdgep->മുകൾ());
                VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                outVertexp->userp(edgep);
            } അല്ലെങ്കിൽ {
                //  cutable duplicates prev  cutable: combine weights
                UINFO(8, "    DelDupComb " << avertexp << " -> " << edgep->മുകൾ());
                prevEdgep->weight(prevEdgep->weight() + edgep->weight());
                addOrigEdgep(prevEdgep, edgep);
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            work_തള്ളുക(outVertexp);
            work_തള്ളുക(avertexp);
        } അല്ലെങ്കിൽ {
            // No previous assignment
            outVertexp->userp(edgep);
        }
    }
}

ശൂന്യം GraphAcyc::cutBasic(GraphAcycVertex* avertexp) {
    // Detect and cleanup any loops from node to itself
    എങ്കിൽ (avertexp->isDelete()) തിരികെ;
    വേണ്ടി (V3GraphEdge* സ്ഥിരം edgep : avertexp->outEdges().unlinkable()) {
        എങ്കിൽ (edgep->cutable() && edgep->മുകൾ() == avertexp) {
            cutOrigEdge(edgep, "  Cut Basic");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_തള്ളുക(avertexp);
        }
    }
}

ശൂന്യം GraphAcyc::cutBackward(GraphAcycVertex* avertexp) {
    // If a cutable edge is from A->B, and there's a non-cutable edge B->A, then must cut!
    എങ്കിൽ (avertexp->isDelete()) തിരികെ;
    // Clear marks
    വേണ്ടി (V3GraphEdge& edge : avertexp->outEdges()) edge.മുകൾ()->user(അസത്യം);
    വേണ്ടി (V3GraphEdge& edge : avertexp->inEdges()) {
        എങ്കിൽ (!edge.cutable()) edge.fromp()->user(സത്യം);
    }
    // Detect duplications
    വേണ്ടി (V3GraphEdge* സ്ഥിരം edgep : avertexp->outEdges().unlinkable()) {
        എങ്കിൽ (edgep->cutable() && edgep->മുകൾ()->user()) {
            cutOrigEdge(edgep, "  Cut A->B->A");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_തള്ളുക(avertexp);
        }
    }
}

ശൂന്യം GraphAcyc::place() {
    // Input is m_breakGraph with ranks already assigned on non-breakable edges

    // Make a list of all cutable edges in the graph
    പൂർണ്ണസംഖ്യ അക്കം_Edges = 0;
    വേണ്ടി (V3GraphVertex& vertex : m_breakGraph.vertices()) {
        വേണ്ടി (V3GraphEdge& edge : vertex.outEdges()) {
            എങ്കിൽ (edge.weight() && edge.cutable()) ++അക്കം_Edges;
        }
    }
    UINFO(4, "    Cutable edges = " << അക്കം_Edges);

    std::vector<V3GraphEdge*> edges;  // List of all edges to be processed
    // Make the vector properly sized right off the bat -- faster than reallocating
    edges.reserve(അക്കം_Edges + 1);
    വേണ്ടി (V3GraphVertex& vertex : m_breakGraph.vertices()) {
        vertex.user(0);  // Clear in prep of next step
        വേണ്ടി (V3GraphEdge& edge : vertex.outEdges()) {
            എങ്കിൽ (edge.weight() && edge.cutable()) edges.തള്ളുക_back(&edge);
        }
    }

    // Sort by weight, then by vertex (so that we completely process one vertex, when possible)
    stable_sort(edges.begin(), edges.end(), GraphAcycEdgeCmp());

    // Process each edge in weighted order
    m_placeStep = 10;
    വേണ്ടി (V3GraphEdge* edgep : edges) placeTryEdge(edgep);
}

ശൂന്യം GraphAcyc::placeTryEdge(V3GraphEdge* edgep) {
    // Try to make this edge uncutable
    m_placeStep++;
    UINFO(8, "    PlaceEdge s" << m_placeStep << " w" << edgep->weight() << " " << edgep->fromp());
    // Make the edge uncutable so we detect it in placement
    edgep->cutable(അസത്യം);
    // Vertex::m_user begin: number indicates this edge was completed
    // Try to assign ranks, presuming this edge is in place
    // If we come across user()==placestep, we've detected a loop and must back out
    സ്ഥിരം തർക്കം loop
        = placeIterate(static_cast<GraphAcycVertex*>(edgep->മുകൾ()), edgep->fromp()->rank() + 1);
    എങ്കിൽ (!loop) {
        // No loop, we can keep it as uncutable
        // Commit the new ranks we calculated
        // Just cleanup the list.  If this is slow, we can add another set of
        // user counters to avoid cleaning up the list.
        ആകുമ്പോൾ (workBeginp()) work_എടുക്കുക();
    } അല്ലെങ്കിൽ {
        // Adding this edge would cause a loop, kill it
        edgep->cutable(സത്യം);  // So graph still looks pretty
        cutOrigEdge(edgep, "  Cut loop");
        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        // Back out the ranks we calculated
        ആകുമ്പോൾ (GraphAcycVertex* vertexp = workBeginp()) {
            work_എടുക്കുക();
            vertexp->rank(vertexp->m_storedRank);
        }
    }
}

തർക്കം GraphAcyc::placeIterate(GraphAcycVertex* vertexp, uint32_t currentRank) {
    // Assign rank to each unvisited node
    //   rank() is the "committed rank" of the graph known without loops
    // If larger rank is found, assign it and loop back through
    // If we hit a back node make a list of all loops
    എങ്കിൽ (vertexp->rank() >= currentRank) തിരികെ അസത്യം;  // Already processed it
    എങ്കിൽ (vertexp->user() == m_placeStep) തിരികെ സത്യം;  // Loop detected
    vertexp->user(m_placeStep);
    // Remember we're changing the rank of this node; might need to back out
    എങ്കിൽ (!vertexp->m_onWorkList) {
        vertexp->m_storedRank = vertexp->rank();
        work_തള്ളുക(vertexp);
    }
    vertexp->rank(currentRank);
    // Follow all edges and increase their ranks
    വേണ്ടി (V3GraphEdge& edge : vertexp->outEdges()) {
        എങ്കിൽ (edge.weight() && !edge.cutable()) {
            എങ്കിൽ (placeIterate(static_cast<GraphAcycVertex*>(edge.മുകൾ()), currentRank + 1)) {
                // We don't need to reset user(); we'll use a different placeStep for the next edge
                തിരികെ സത്യം;  // Loop detected
            }
        }
    }
    vertexp->user(0);
    തിരികെ അസത്യം;
}

//----- Main algorithm entry point

ശൂന്യം GraphAcyc::പ്രധാന() {
    m_breakGraph.userClearEdges();

    // Color based on possible loops
    m_origGraphp->stronglyConnected(m_origEdgeFuncp);

    // Make a new graph with vertices that have only a single vertex
    // for each group of old vertices that are interconnected with unbreakable
    // edges (and thus can't represent loops - if we did the unbreakable
    // marking right, anyways)
    buildGraph(m_origGraphp);
    എങ്കിൽ (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_pre");

    // Perform simple optimizations before any cuttings
    simplify(അസത്യം);
    എങ്കിൽ (dumpGraphLevel() >= 5) m_breakGraph.dumpDotFilePrefixed("acyc_simp");

    UINFO(4, " Cutting trivial loops");
    simplify(സത്യം);
    എങ്കിൽ (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_mid");

    UINFO(4, " Ranking");
    m_breakGraph.rank(&V3GraphEdge::followNotCutable);
    എങ്കിൽ (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_rank");

    UINFO(4, " Placement");
    place();
    എങ്കിൽ (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_place");

    UINFO(4, " Final Ranking");
    // Only needed to assert there are no loops in completed graph
    m_breakGraph.rank(&V3GraphEdge::followAlwaysTrue);
    എങ്കിൽ (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_done");
}

ശൂന്യം V3Graph::acyclic(V3EdgeFuncP edgeFuncp) {
    UINFO(4, "Acyclic");
    GraphAcyc acyc{ഇത്, edgeFuncp};
    acyc.പ്രധാന();
    UINFO(4, "Acyclic done");
}
