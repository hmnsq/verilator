// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Graph optimizations
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

#समावेश "V3GraphAlg.h"

#समावेश "V3Global.h"
#समावेश "V3GraphPathChecker.h"
#समावेश "V3GraphStream.h"
#समावेश "V3Stats.h"

#समावेश <algorithm>
#समावेश <limits>
#समावेश <list>
#समावेश <map>
#समावेश <numeric>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - Remove redundancies
// Changes user() and weight()

श्रेणी GraphRemoveRedundant final : GraphAlg<> {
    स्थिर bool m_योग_Weights;  // Sum, rather then maximize weights
निजी:
    व्योम मुख्य() {
        क्रम (V3GraphVertex& vertex : m_graphp->vertices()) vertexIterate(&vertex);
    }
    व्योम vertexIterate(V3GraphVertex* vertexp) {
        // Clear marks
        क्रम (V3GraphEdge& edge : vertexp->outEdges()) edge.शीर्ष()->userp(nullptr);

        // Mark edges and detect duplications

        क्रम (V3GraphEdge* स्थिर edgep : vertexp->outEdges().unlinkable()) {
            अगर (followEdge(edgep)) {
                V3GraphVertex* outVertexp = edgep->शीर्ष();
                V3GraphEdge* पूर्व_Edgep = static_cast<V3GraphEdge*>(outVertexp->userp());
                अगर (!पूर्व_Edgep) {  // No previous assignment
                    outVertexp->userp(edgep);
                } अन्यथा {  // Duplicate
                    bool saveOld = true;
                    अगर (पूर्व_Edgep->cutable() && !edgep->cutable()) {
                        saveOld = false;  // new !cutable more important than old
                    } अन्यथा अगर (!पूर्व_Edgep->cutable() && edgep->cutable()) {
                        saveOld = true;  // old !cutable more important than new
                    } अन्यथा {
                        saveOld = true;
                        अगर (!m_योग_Weights
                            && (पूर्व_Edgep->weight() < edgep->weight())) {  // Keep max weight
                            पूर्व_Edgep->weight(edgep->weight());
                        }
                    }
                    अगर (saveOld) {
                        अगर (m_योग_Weights) पूर्व_Edgep->weight(पूर्व_Edgep->weight() + edgep->weight());
                        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    } अन्यथा {
                        अगर (m_योग_Weights) edgep->weight(पूर्व_Edgep->weight() + edgep->weight());
                        VL_DO_DANGLING(पूर्व_Edgep->unlinkDelete(), पूर्व_Edgep);
                        outVertexp->userp(edgep);
                    }
                }
            }
        }
    }

खुला:
    GraphRemoveRedundant(V3Graph* graphp, V3EdgeFuncP edgeFuncp, bool योग_Weights)
        : GraphAlg<>{graphp, edgeFuncp}
        , m_योग_Weights{योग_Weights} {
        मुख्य();
    }
    ~GraphRemoveRedundant() = शेष;
};

व्योम V3Graph::removeRedundantEdgesMax(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{यह, edgeFuncp, false};
}
व्योम V3Graph::remove_Redundant_Edges_योग(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{यह, edgeFuncp, true};
}

//######################################################################
//######################################################################
// Algorithms - remove transitive

श्रेणी GraphAlgRemoveTransitiveEdges final : GraphAlg<> {
खुला:
    explicit GraphAlgRemoveTransitiveEdges(V3Graph* graphp)
        : GraphAlg<>(graphp, nullptr) {}
    व्योम go() {
        GraphPathChecker checker{m_graphp};
        क्रम (V3GraphVertex& vtx : m_graphp->vertices()) {
            V3GraphEdge* deletep = nullptr;
            क्रम (V3GraphEdge& edge : vtx.outEdges()) {
                अगर (deletep) VL_DO_CLEAR(deletep->unlinkDelete(), deletep = nullptr);
                // It should be safe to modify the graph, despite using
                // the GraphPathChecker, as none of the modifications will
                // change what can be reached from what, nor should they
                // change the rank or CP of any node.
                अगर (checker.isTransitiveEdge(&edge)) deletep = &edge;
            }
            अगर (deletep) VL_DO_DANGLING(deletep->unlinkDelete(), deletep);
        }
    }

निजी:
    VL_UNCOPYABLE(GraphAlgRemoveTransitiveEdges);
};

व्योम V3Graph::removeTransitiveEdges() { GraphAlgRemoveTransitiveEdges{यह}.go(); }

//######################################################################
//######################################################################
// Algorithms - weakly connected components
// Changes color()

श्रेणी GraphAlgWeakly final : GraphAlg<> {
    // Sentinel color, meaning not colored yet. Colors themselves are 0 .. m_numColors-1.
    अटल constexpr uint32_t UNCOLORED = std::numeric_limits<uint32_t>::max();

    uint32_t m_अंक_Colors = 0;  // Number of colors assigned

    व्योम मुख्य() {
        // Initialize state
        क्रम (V3GraphVertex& vertex : m_graphp->vertices()) vertex.color(UNCOLORED);
        // Color graph, without gaps
        क्रम (V3GraphVertex& vertex : m_graphp->vertices()) {
            अगर (vertex.color() == UNCOLORED) vertexIterate(&vertex, m_अंक_Colors++);
        }
    }

    व्योम vertexIterate(V3GraphVertex* vertexp, uint32_t currentColor) {
        // Assign new color to each unvisited node
        // then visit each of its edges, giving them the same color
        अगर (vertexp->color() != UNCOLORED) वापस;  // Already colored it
        vertexp->color(currentColor);
        क्रम (V3GraphEdge& edge : vertexp->outEdges()) {
            अगर (followEdge(&edge)) vertexIterate(edge.शीर्ष(), currentColor);
        }
        क्रम (V3GraphEdge& edge : vertexp->inEdges()) {
            अगर (followEdge(&edge)) vertexIterate(edge.fromp(), currentColor);
        }
    }

खुला:
    GraphAlgWeakly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>(graphp, edgeFuncp) {
        मुख्य();
    }
    ~GraphAlgWeakly() = शेष;

    uint32_t अंक_Colors() स्थिर { वापस m_अंक_Colors; }
};

uint32_t V3Graph::weaklyConnected(V3EdgeFuncP edgeFuncp) {
    वापस GraphAlgWeakly{यह, edgeFuncp}.अंक_Colors();
}

//######################################################################
//######################################################################
// Algorithms - strongly connected components
// Changes user() and color()

श्रेणी GraphAlgStrongly final : GraphAlg<> {
    uint32_t m_currentDfs = 0;  // DFS count
    std::vector<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far

    व्योम मुख्य() {
        // Use Pearce's algorithm to color the strongly connected components. For reference see
        // "An Improved Algorithm for Finding the Strongly Connected Components of a Directed
        // Graph", David J.Pearce, 2005
        //
        // Node State:
        //     Vertex::user     // DFS number indicating possible root of subtree, 0=not iterated
        //     Vertex::color    // Output subtree number (fully processed)

        // Clear info
        क्रम (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.color(0);
            vertex.user(0);
        }
        // Color graph
        क्रम (V3GraphVertex& vertex : m_graphp->vertices()) {
            अगर (!vertex.user()) {
                m_currentDfs++;
                vertexIterate(&vertex);
            }
        }
        // If there's a single vertex of a color, it doesn't need a subgraph
        // This simplifies the consumer's code, and reduces graph debugging clutter
        क्रम (V3GraphVertex& vertex : m_graphp->vertices()) {
            bool onecolor = true;
            क्रम (V3GraphEdge& edge : vertex.outEdges()) {
                अगर (followEdge(&edge)) {
                    अगर (vertex.color() == edge.शीर्ष()->color()) {
                        onecolor = false;
                        अवरोध;
                    }
                }
            }
            अगर (onecolor) vertex.color(0);
        }
    }

    व्योम vertexIterate(V3GraphVertex* vertexp) {
        स्थिर uint32_t this_Dfs_अंक = m_currentDfs++;
        vertexp->user(this_Dfs_अंक);
        vertexp->color(0);
        क्रम (V3GraphEdge& edge : vertexp->outEdges()) {
            अगर (followEdge(&edge)) {
                V3GraphVertex* शीर्ष = edge.शीर्ष();
                अगर (!शीर्ष->user()) {  // Dest not computed yet
                    vertexIterate(शीर्ष);
                }
                अगर (!शीर्ष->color()) {  // Dest not in a component
                    अगर (vertexp->user() > शीर्ष->user()) vertexp->user(शीर्ष->user());
                }
            }
        }
        अगर (vertexp->user() == this_Dfs_अंक) {  // New head of subtree
            vertexp->color(this_Dfs_अंक);  // Mark as component
            जबतक (!m_callTrace.empty()) {
                V3GraphVertex* निकालो_Vertexp = m_callTrace.back();
                अगर (निकालो_Vertexp->user() >= this_Dfs_अंक) {  // Lower node is part of this subtree
                    m_callTrace.निकालो_back();
                    निकालो_Vertexp->color(this_Dfs_अंक);
                } अन्यथा {
                    अवरोध;
                }
            }
        } अन्यथा {  // In another subtree (maybe...)
            m_callTrace.धकेलो_back(vertexp);
        }
    }

खुला:
    GraphAlgStrongly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        मुख्य();
    }
    ~GraphAlgStrongly() = शेष;
};

व्योम V3Graph::stronglyConnected(V3EdgeFuncP edgeFuncp) { GraphAlgStrongly{यह, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking
// Changes user() and rank()

श्रेणी GraphAlgRank final : GraphAlg<> {
    व्योम मुख्य() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        क्रम (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        क्रम (V3GraphVertex& vertex : m_graphp->vertices()) {
            अगर (!vertex.user()) vertexIterate(&vertex, 1);
        }
    }
    व्योम vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // If larger rank is found, assign it and loop back through
        // If we hit a back node make a list of all loops
        अगर (vertexp->user() == 1) {
            m_graphp->loopsMessageCb(vertexp, m_edgeFuncp);
            वापस;  // LCOV_EXCL_LINE  // gcc gprof bug misses this return
        }
        अगर (vertexp->rank() >= currentRank) वापस;  // Already processed it
        vertexp->user(1);
        vertexp->rank(currentRank);
        क्रम (V3GraphEdge& edge : vertexp->outEdges()) {
            अगर (followEdge(&edge)) vertexIterate(edge.शीर्ष(), currentRank + vertexp->rankAdder());
        }
        vertexp->user(2);
    }

खुला:
    GraphAlgRank(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        मुख्य();
    }
    ~GraphAlgRank() = शेष;
};

व्योम V3Graph::rank() { rank(&V3GraphEdge::followAlwaysTrue); }
व्योम V3Graph::rank(V3EdgeFuncP edgeFuncp) { GraphAlgRank{यह, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking min
// Changes user() and rank()

श्रेणी GraphAlgRankMin final : GraphAlg<> {
    व्योम मुख्य() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        क्रम (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        क्रम (V3GraphVertex& vertex : m_graphp->vertices()) {
            अगर (!vertex.user()) vertexIterate(&vertex);
        }
    }
    uint32_t vertexIterate(V3GraphVertex* vertexp) {
        // Assign rank to each unvisited node
        // If we hit a back node make a list of all loops
        अगर (vertexp->user() == 1) {
            m_graphp->loopsMessageCb(vertexp, m_edgeFuncp);
            वापस vertexp->rank();
        }
        अगर (vertexp->user()) वापस vertexp->rank();  // Done earlier
        vertexp->user(1);
        vertexp->rank(1);  // In case loop
        // If no input edges, then rank 1 (+ adder)
        // Otherwise, get minimum from following all inputs.
        uint32_t minrank = ~0U;
        क्रम (V3GraphEdge& edge : vertexp->inEdges()) {
            अगर (followEdge(&edge)) {
                स्थिर uint32_t nrank = vertexIterate(edge.fromp());
                minrank = std::min(minrank, nrank);
            }
        }
        अगर (minrank == ~0U) minrank = 0;
        vertexp->rank(minrank + vertexp->rankAdder());
        vertexp->user(2);
        वापस vertexp->rank();
    }

खुला:
    GraphAlgRankMin(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        मुख्य();
    }
    ~GraphAlgRankMin() = शेष;
};

व्योम V3Graph::rankMin() { rankMin(&V3GraphEdge::followAlwaysTrue); }
व्योम V3Graph::rankMin(V3EdgeFuncP edgeFuncp) { GraphAlgRankMin{यह, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - report loops
// Changes user()

श्रेणी GraphAlgRLoops final : GraphAlg<> {
    std::vector<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far
    std::vector<string> m_msgs;  // Output messages
    bool m_done = false;  // Exit algorithm

    व्योम मुख्य(V3GraphVertex* vertexp) {
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        m_graphp->userClearVertices();
        m_callTrace.reserve(100);
        vertexIterate(vertexp, 0);
    }

    व्योम vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // When we hit ourself again, return the list of all loops
        अगर (m_done) वापस;

        // Can't just reserve(), unless we modify size() before setting array directly
        जबतक (m_callTrace.आकार() <= currentRank) m_callTrace.धकेलो_back(vertexp);
        m_callTrace[currentRank++] = vertexp;

        अगर (vertexp->user() == 1) {
            क्रम (अचिन्हित i = 0; i < currentRank; i++)
                m_msgs.emplace_back(m_graphp->loopsVertexCb(m_callTrace[i]));
            m_done = true;
            वापस;
        }
        अगर (vertexp->user() == 2) वापस;  // Already processed it
        vertexp->user(1);
        क्रम (V3GraphEdge& edge : vertexp->outEdges()) {
            अगर (followEdge(&edge)) vertexIterate(edge.शीर्ष(), currentRank);
        }
        vertexp->user(2);
    }

खुला:
    GraphAlgRLoops(V3Graph* graphp, V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp)
        : GraphAlg<>{graphp, edgeFuncp} {
        मुख्य(vertexp);
    }
    ~GraphAlgRLoops() = शेष;
    string message() स्थिर {
        वापस std::accumulate(m_msgs.begin(), m_msgs.end(), std::string{""});
    }
};

string V3Graph::reportLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp) {
    वापस GraphAlgRLoops{यह, edgeFuncp, vertexp}.message();
}

//######################################################################
//######################################################################
// Algorithms - subtrees
// Changes user()

श्रेणी GraphAlgSubtrees final : GraphAlg<> {
    V3Graph* स्थिर m_loopGraphp;

    //! Iterate through all connected nodes of a graph with a loop or loops.
    V3GraphVertex* vertexIterateAll(V3GraphVertex* vertexp) {
        अगर (V3GraphVertex* newVertexp = static_cast<V3GraphVertex*>(vertexp->userp())) {
            वापस newVertexp;
        } अन्यथा {
            newVertexp = vertexp->clone(m_loopGraphp);
            vertexp->userp(newVertexp);

            क्रम (V3GraphEdge& edge : vertexp->outEdges()) {
                अगर (followEdge(&edge)) {
                    V3GraphEdge* newEdgep = static_cast<V3GraphEdge*>(edge.userp());
                    अगर (!newEdgep) {
                        V3GraphVertex* new_शीर्ष = vertexIterateAll(edge.शीर्ष());
                        newEdgep = edge.clone(m_loopGraphp, newVertexp, new_शीर्ष);
                        edge.userp(newEdgep);
                    }
                }
            }
            वापस newVertexp;
        }
    }

खुला:
    GraphAlgSubtrees(V3Graph* graphp, V3Graph* loopGraphp, V3EdgeFuncP edgeFuncp,
                     V3GraphVertex* vertexp)
        : GraphAlg<>{graphp, edgeFuncp}
        , m_loopGraphp{loopGraphp} {
        // Vertex::m_userp - New vertex if we have seen this vertex already
        // Edge::m_userp - New edge if we have seen this edge already
        m_graphp->userClearVertices();
        m_graphp->userClearEdges();
        (व्योम)vertexIterateAll(vertexp);
    }
    ~GraphAlgSubtrees() = शेष;
};

//! Report the entire connected graph with a loop or loops
व्योम V3Graph::subtreeLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp, V3Graph* loopGraphp) {
    GraphAlgSubtrees{यह, loopGraphp, edgeFuncp, vertexp};
}

//######################################################################
//######################################################################
// Algorithms - sorting

व्योम V3Graph::sortVertices() {
    // Sort list of vertices by rank, then fanout
    std::vector<V3GraphVertex*> vertexps;
    क्रम (V3GraphVertex& vertex : m_vertices) vertexps.धकेलो_back(&vertex);
    std::stable_sort(vertexps.begin(), vertexps.end(),
                     [](स्थिर V3GraphVertex* lhsp, स्थिर V3GraphVertex* rhsp) {  //
                         वापस lhsp->sortCmp(rhsp) < 0;
                     });
    // Re-insert in sorted order
    क्रम (V3GraphVertex* स्थिर vertexp : vertexps) {
        m_vertices.unlink(vertexp);
        m_vertices.linkBack(vertexp);
    }
}

व्योम V3Graph::sortEdges() {
    // Sort edges by rank then fanout of node they point to
    std::vector<V3GraphEdge*> edges;
    क्रम (V3GraphVertex& vertex : vertices()) {
        // Make a vector
        क्रम (V3GraphEdge& edge : vertex.outEdges()) edges.धकेलो_back(&edge);
        // Sort
        std::stable_sort(edges.begin(), edges.end(),
                         [](स्थिर V3GraphEdge* lhsp, स्थिर V3GraphEdge* rhsp) {  //
                             वापस lhsp->sortCmp(rhsp) < 0;
                         });
        // Relink edges in specified order
        क्रम (V3GraphEdge* स्थिर edgep : edges) edgep->relinkFromp(&vertex);
        // Prep for next
        edges.सफा();
    }
}

//######################################################################
//######################################################################
// Algorithms - ordering
//      Compute near optimal ordering of the nodes, where:
//          If a required    edge is A->B, rank(A)<rank(B)
//          Visit edges and assign ranks to keep minimal crossings
//              (Results in better dcache packing.)

व्योम V3Graph::order() {
    UINFO(2, "Order:");

    // Compute rankings again
    rank(&V3GraphEdge::followAlwaysTrue);
    orderPreRanked();
}

व्योम V3Graph::orderPreRanked() {
    // Compute fanouts
    // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
    userClearVertices();
    क्रम (V3GraphVertex& vertex : vertices()) {
        अगर (!vertex.user()) orderDFSIterate(&vertex);
    }

    // Sort list of vertices by rank, then fanout. Fanout is a bit of a
    // misnomer. It is the sum of all the fanouts of nodes reached from a node
    // *plus* the count of edges in to that node.
    sortVertices();
    // Sort edges by rank then fanout of node they point to
    sortEdges();
}

द्विगुन V3Graph::orderDFSIterate(V3GraphVertex* vertexp) {
    // Compute fanouts of each node
    // If forward edge, don't double count that fanout
    अगर (vertexp->user() == 2) वापस vertexp->fanout();  // Already processed it
    UASSERT_OBJ(vertexp->user() != 1, vertexp, "Loop found, backward edges should be dead");
    vertexp->user(1);
    द्विगुन fanout = 0;
    क्रम (V3GraphEdge& edge : vertexp->outEdges()) {
        अगर (edge.weight()) fanout += orderDFSIterate(edge.शीर्ष());
    }
    // Just count inbound edges
    क्रम (V3GraphEdge& edge : vertexp->inEdges()) {
        अगर (edge.weight()) ++fanout;
    }
    vertexp->fanout(fanout);
    vertexp->user(2);
    वापस vertexp->fanout();
}

//######################################################################
//######################################################################
// Algorithms - parallelism report

श्रेणी GraphAlgParallelismReport final {
    // MEMBERS
    V3Graph& m_graph;  // The graph
    स्थिर std::function<uint64_t(स्थिर V3GraphVertex*)> m_vertexCost;  // vertex cost function
    V3Graph::ParallelismReport m_report;  // The result report

    // CONSTRUCTORS
    explicit GraphAlgParallelismReport(V3Graph& graph,
                                       std::function<uint64_t(स्थिर V3GraphVertex*)> vertexCost)
        : m_graph{graph}
        , m_vertexCost{vertexCost} {
        // For each node, record the critical path cost from the start
        // of the graph through the end of the node.
        std::unordered_map<स्थिर V3GraphVertex*, uint64_t> critPaths;
        GraphStreamUnordered serialize{&m_graph};
        क्रम (स्थिर V3GraphVertex* vertexp; (vertexp = serialize.nextp());) {
            ++m_report.m_vertex_गिनती;
            uint64_t cpCostToHere = 0;
            क्रम (स्थिर V3GraphEdge& edge : vertexp->inEdges()) {
                ++m_report.m_edge_गिनती;
                // For each upstream item, add its critical path cost to
                // the cost of this edge, to form a new candidate critical
                // path cost to the current node. Whichever is largest is
                // the critical path to reach the start of this node.
                cpCostToHere = std::max(cpCostToHere, critPaths[edge.fromp()]);
            }
            // Include the cost of the current vertex in the critical
            // path, so it represents the critical path to the end of
            // this vertex.
            cpCostToHere += m_vertexCost(vertexp);
            critPaths[vertexp] = cpCostToHere;
            m_report.m_criticalPathCost = std::max(m_report.m_criticalPathCost, cpCostToHere);
            // Tally the total cost contributed by vertices.
            m_report.m_योग_Graph_Cost += m_vertexCost(vertexp);
        }
    }
    ~GraphAlgParallelismReport() = शेष;
    VL_UNCOPYABLE(GraphAlgParallelismReport);
    VL_UNMOVABLE(GraphAlgParallelismReport);

खुला:
    अटल V3Graph::ParallelismReport
    apply(V3Graph& graph, std::function<uint32_t(स्थिर V3GraphVertex*)> vertexCost) {
        वापस GraphAlgParallelismReport(graph, vertexCost).m_report;
    }
};

V3Graph::ParallelismReport
V3Graph::parallelismReport(std::function<uint64_t(स्थिर V3GraphVertex*)> vertexCost) {
    वापस GraphAlgParallelismReport::apply(*यह, vertexCost);
}
