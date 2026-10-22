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

#define VL_MT_DISABLED_CODE_UNIT 1

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3GraphAlg.h"

#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3GraphPathChecker.h"
#અન્તર્ભાવ "V3GraphStream.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <limits>
#અન્તર્ભાવ <list>
#અન્તર્ભાવ <map>
#અન્તર્ભાવ <numeric>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - Remove redundancies
// Changes user() and weight()

વર્ગ GraphRemoveRedundant final : GraphAlg<> {
    અચલ તાર્કિક m_sumWeights;  // Sum, rather then maximize weights
ખાનગી:
    શૂન્ય મુખ્ય() {
        માટે (V3GraphVertex& vertex : m_graphp->vertices()) vertexIterate(&vertex);
    }
    શૂન્ય vertexIterate(V3GraphVertex* vertexp) {
        // Clear marks
        માટે (V3GraphEdge& edge : vertexp->outEdges()) edge.ટોચ()->userp(nullptr);

        // Mark edges and detect duplications

        માટે (V3GraphEdge* અચલ edgep : vertexp->outEdges().unlinkable()) {
            જો (followEdge(edgep)) {
                V3GraphVertex* outVertexp = edgep->ટોચ();
                V3GraphEdge* prevEdgep = static_cast<V3GraphEdge*>(outVertexp->userp());
                જો (!prevEdgep) {  // No previous assignment
                    outVertexp->userp(edgep);
                } નહીંતર {  // Duplicate
                    તાર્કિક saveOld = સાચું;
                    જો (prevEdgep->cutable() && !edgep->cutable()) {
                        saveOld = ખોટું;  // new !cutable more important than old
                    } નહીંતર જો (!prevEdgep->cutable() && edgep->cutable()) {
                        saveOld = સાચું;  // old !cutable more important than new
                    } નહીંતર {
                        saveOld = સાચું;
                        જો (!m_sumWeights
                            && (prevEdgep->weight() < edgep->weight())) {  // Keep max weight
                            prevEdgep->weight(edgep->weight());
                        }
                    }
                    જો (saveOld) {
                        જો (m_sumWeights) prevEdgep->weight(prevEdgep->weight() + edgep->weight());
                        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    } નહીંતર {
                        જો (m_sumWeights) edgep->weight(prevEdgep->weight() + edgep->weight());
                        VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                        outVertexp->userp(edgep);
                    }
                }
            }
        }
    }

જાહેર:
    GraphRemoveRedundant(V3Graph* graphp, V3EdgeFuncP edgeFuncp, તાર્કિક sumWeights)
        : GraphAlg<>{graphp, edgeFuncp}
        , m_sumWeights{sumWeights} {
        મુખ્ય();
    }
    ~GraphRemoveRedundant() = મૂળભૂત;
};

શૂન્ય V3Graph::removeRedundantEdgesMax(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{આ, edgeFuncp, ખોટું};
}
શૂન્ય V3Graph::removeRedundantEdgesSum(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{આ, edgeFuncp, સાચું};
}

//######################################################################
//######################################################################
// Algorithms - remove transitive

વર્ગ GraphAlgRemoveTransitiveEdges final : GraphAlg<> {
જાહેર:
    explicit GraphAlgRemoveTransitiveEdges(V3Graph* graphp)
        : GraphAlg<>(graphp, nullptr) {}
    શૂન્ય go() {
        GraphPathChecker checker{m_graphp};
        માટે (V3GraphVertex& vtx : m_graphp->vertices()) {
            V3GraphEdge* deletep = nullptr;
            માટે (V3GraphEdge& edge : vtx.outEdges()) {
                જો (deletep) VL_DO_CLEAR(deletep->unlinkDelete(), deletep = nullptr);
                // It should be safe to modify the graph, despite using
                // the GraphPathChecker, as none of the modifications will
                // change what can be reached from what, nor should they
                // change the rank or CP of any node.
                જો (checker.isTransitiveEdge(&edge)) deletep = &edge;
            }
            જો (deletep) VL_DO_DANGLING(deletep->unlinkDelete(), deletep);
        }
    }

ખાનગી:
    VL_UNCOPYABLE(GraphAlgRemoveTransitiveEdges);
};

શૂન્ય V3Graph::removeTransitiveEdges() { GraphAlgRemoveTransitiveEdges{આ}.go(); }

//######################################################################
//######################################################################
// Algorithms - weakly connected components
// Changes color()

વર્ગ GraphAlgWeakly final : GraphAlg<> {
    // Sentinel color, meaning not colored yet. Colors themselves are 0 .. m_numColors-1.
    સ્થિર constexpr uint32_t UNCOLORED = std::numeric_limits<uint32_t>::max();

    uint32_t m_અંક_Colors = 0;  // Number of colors assigned

    શૂન્ય મુખ્ય() {
        // Initialize state
        માટે (V3GraphVertex& vertex : m_graphp->vertices()) vertex.color(UNCOLORED);
        // Color graph, without gaps
        માટે (V3GraphVertex& vertex : m_graphp->vertices()) {
            જો (vertex.color() == UNCOLORED) vertexIterate(&vertex, m_અંક_Colors++);
        }
    }

    શૂન્ય vertexIterate(V3GraphVertex* vertexp, uint32_t currentColor) {
        // Assign new color to each unvisited node
        // then visit each of its edges, giving them the same color
        જો (vertexp->color() != UNCOLORED) પાછા;  // Already colored it
        vertexp->color(currentColor);
        માટે (V3GraphEdge& edge : vertexp->outEdges()) {
            જો (followEdge(&edge)) vertexIterate(edge.ટોચ(), currentColor);
        }
        માટે (V3GraphEdge& edge : vertexp->inEdges()) {
            જો (followEdge(&edge)) vertexIterate(edge.fromp(), currentColor);
        }
    }

જાહેર:
    GraphAlgWeakly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>(graphp, edgeFuncp) {
        મુખ્ય();
    }
    ~GraphAlgWeakly() = મૂળભૂત;

    uint32_t અંક_Colors() અચલ { પાછા m_અંક_Colors; }
};

uint32_t V3Graph::weaklyConnected(V3EdgeFuncP edgeFuncp) {
    પાછા GraphAlgWeakly{આ, edgeFuncp}.અંક_Colors();
}

//######################################################################
//######################################################################
// Algorithms - strongly connected components
// Changes user() and color()

વર્ગ GraphAlgStrongly final : GraphAlg<> {
    uint32_t m_currentDfs = 0;  // DFS count
    std::vector<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far

    શૂન્ય મુખ્ય() {
        // Use Pearce's algorithm to color the strongly connected components. For reference see
        // "An Improved Algorithm for Finding the Strongly Connected Components of a Directed
        // Graph", David J.Pearce, 2005
        //
        // Node State:
        //     Vertex::user     // DFS number indicating possible root of subtree, 0=not iterated
        //     Vertex::color    // Output subtree number (fully processed)

        // Clear info
        માટે (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.color(0);
            vertex.user(0);
        }
        // Color graph
        માટે (V3GraphVertex& vertex : m_graphp->vertices()) {
            જો (!vertex.user()) {
                m_currentDfs++;
                vertexIterate(&vertex);
            }
        }
        // If there's a single vertex of a color, it doesn't need a subgraph
        // This simplifies the consumer's code, and reduces graph debugging clutter
        માટે (V3GraphVertex& vertex : m_graphp->vertices()) {
            તાર્કિક onecolor = સાચું;
            માટે (V3GraphEdge& edge : vertex.outEdges()) {
                જો (followEdge(&edge)) {
                    જો (vertex.color() == edge.ટોચ()->color()) {
                        onecolor = ખોટું;
                        તોડો;
                    }
                }
            }
            જો (onecolor) vertex.color(0);
        }
    }

    શૂન્ય vertexIterate(V3GraphVertex* vertexp) {
        અચલ uint32_t this_Dfs_અંક = m_currentDfs++;
        vertexp->user(this_Dfs_અંક);
        vertexp->color(0);
        માટે (V3GraphEdge& edge : vertexp->outEdges()) {
            જો (followEdge(&edge)) {
                V3GraphVertex* ટોચ = edge.ટોચ();
                જો (!ટોચ->user()) {  // Dest not computed yet
                    vertexIterate(ટોચ);
                }
                જો (!ટોચ->color()) {  // Dest not in a component
                    જો (vertexp->user() > ટોચ->user()) vertexp->user(ટોચ->user());
                }
            }
        }
        જો (vertexp->user() == this_Dfs_અંક) {  // New head of subtree
            vertexp->color(this_Dfs_અંક);  // Mark as component
            જ્યારે (!m_callTrace.empty()) {
                V3GraphVertex* કાઢો_Vertexp = m_callTrace.back();
                જો (કાઢો_Vertexp->user() >= this_Dfs_અંક) {  // Lower node is part of this subtree
                    m_callTrace.કાઢો_back();
                    કાઢો_Vertexp->color(this_Dfs_અંક);
                } નહીંતર {
                    તોડો;
                }
            }
        } નહીંતર {  // In another subtree (maybe...)
            m_callTrace.ધકેલો_back(vertexp);
        }
    }

જાહેર:
    GraphAlgStrongly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        મુખ્ય();
    }
    ~GraphAlgStrongly() = મૂળભૂત;
};

શૂન્ય V3Graph::stronglyConnected(V3EdgeFuncP edgeFuncp) { GraphAlgStrongly{આ, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking
// Changes user() and rank()

વર્ગ GraphAlgRank final : GraphAlg<> {
    શૂન્ય મુખ્ય() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        માટે (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        માટે (V3GraphVertex& vertex : m_graphp->vertices()) {
            જો (!vertex.user()) vertexIterate(&vertex, 1);
        }
    }
    શૂન્ય vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // If larger rank is found, assign it and loop back through
        // If we hit a back node make a list of all loops
        જો (vertexp->user() == 1) {
            m_graphp->loopsMessageCb(vertexp, m_edgeFuncp);
            પાછા;  // LCOV_EXCL_LINE  // gcc gprof bug misses this return
        }
        જો (vertexp->rank() >= currentRank) પાછા;  // Already processed it
        vertexp->user(1);
        vertexp->rank(currentRank);
        માટે (V3GraphEdge& edge : vertexp->outEdges()) {
            જો (followEdge(&edge)) vertexIterate(edge.ટોચ(), currentRank + vertexp->rankAdder());
        }
        vertexp->user(2);
    }

જાહેર:
    GraphAlgRank(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        મુખ્ય();
    }
    ~GraphAlgRank() = મૂળભૂત;
};

શૂન્ય V3Graph::rank() { rank(&V3GraphEdge::followAlwaysTrue); }
શૂન્ય V3Graph::rank(V3EdgeFuncP edgeFuncp) { GraphAlgRank{આ, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking min
// Changes user() and rank()

વર્ગ GraphAlgRankMin final : GraphAlg<> {
    શૂન્ય મુખ્ય() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        માટે (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        માટે (V3GraphVertex& vertex : m_graphp->vertices()) {
            જો (!vertex.user()) vertexIterate(&vertex);
        }
    }
    uint32_t vertexIterate(V3GraphVertex* vertexp) {
        // Assign rank to each unvisited node
        // If we hit a back node make a list of all loops
        જો (vertexp->user() == 1) {
            m_graphp->loopsMessageCb(vertexp, m_edgeFuncp);
            પાછા vertexp->rank();
        }
        જો (vertexp->user()) પાછા vertexp->rank();  // Done earlier
        vertexp->user(1);
        vertexp->rank(1);  // In case loop
        // If no input edges, then rank 1 (+ adder)
        // Otherwise, get minimum from following all inputs.
        uint32_t minrank = ~0U;
        માટે (V3GraphEdge& edge : vertexp->inEdges()) {
            જો (followEdge(&edge)) {
                અચલ uint32_t nrank = vertexIterate(edge.fromp());
                minrank = std::min(minrank, nrank);
            }
        }
        જો (minrank == ~0U) minrank = 0;
        vertexp->rank(minrank + vertexp->rankAdder());
        vertexp->user(2);
        પાછા vertexp->rank();
    }

જાહેર:
    GraphAlgRankMin(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        મુખ્ય();
    }
    ~GraphAlgRankMin() = મૂળભૂત;
};

શૂન્ય V3Graph::rankMin() { rankMin(&V3GraphEdge::followAlwaysTrue); }
શૂન્ય V3Graph::rankMin(V3EdgeFuncP edgeFuncp) { GraphAlgRankMin{આ, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - report loops
// Changes user()

વર્ગ GraphAlgRLoops final : GraphAlg<> {
    std::vector<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far
    std::vector<string> m_msgs;  // Output messages
    તાર્કિક m_done = ખોટું;  // Exit algorithm

    શૂન્ય મુખ્ય(V3GraphVertex* vertexp) {
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        m_graphp->userClearVertices();
        m_callTrace.reserve(100);
        vertexIterate(vertexp, 0);
    }

    શૂન્ય vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // When we hit ourself again, return the list of all loops
        જો (m_done) પાછા;

        // Can't just reserve(), unless we modify size() before setting array directly
        જ્યારે (m_callTrace.size() <= currentRank) m_callTrace.ધકેલો_back(vertexp);
        m_callTrace[currentRank++] = vertexp;

        જો (vertexp->user() == 1) {
            માટે (unsigned i = 0; i < currentRank; i++)
                m_msgs.emplace_back(m_graphp->loopsVertexCb(m_callTrace[i]));
            m_done = સાચું;
            પાછા;
        }
        જો (vertexp->user() == 2) પાછા;  // Already processed it
        vertexp->user(1);
        માટે (V3GraphEdge& edge : vertexp->outEdges()) {
            જો (followEdge(&edge)) vertexIterate(edge.ટોચ(), currentRank);
        }
        vertexp->user(2);
    }

જાહેર:
    GraphAlgRLoops(V3Graph* graphp, V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp)
        : GraphAlg<>{graphp, edgeFuncp} {
        મુખ્ય(vertexp);
    }
    ~GraphAlgRLoops() = મૂળભૂત;
    string message() અચલ {
        પાછા std::accumulate(m_msgs.begin(), m_msgs.end(), std::string{""});
    }
};

string V3Graph::reportLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp) {
    પાછા GraphAlgRLoops{આ, edgeFuncp, vertexp}.message();
}

//######################################################################
//######################################################################
// Algorithms - subtrees
// Changes user()

વર્ગ GraphAlgSubtrees final : GraphAlg<> {
    V3Graph* અચલ m_loopGraphp;

    //! Iterate through all connected nodes of a graph with a loop or loops.
    V3GraphVertex* vertexIterateAll(V3GraphVertex* vertexp) {
        જો (V3GraphVertex* newVertexp = static_cast<V3GraphVertex*>(vertexp->userp())) {
            પાછા newVertexp;
        } નહીંતર {
            newVertexp = vertexp->clone(m_loopGraphp);
            vertexp->userp(newVertexp);

            માટે (V3GraphEdge& edge : vertexp->outEdges()) {
                જો (followEdge(&edge)) {
                    V3GraphEdge* newEdgep = static_cast<V3GraphEdge*>(edge.userp());
                    જો (!newEdgep) {
                        V3GraphVertex* new_ટોચ = vertexIterateAll(edge.ટોચ());
                        newEdgep = edge.clone(m_loopGraphp, newVertexp, new_ટોચ);
                        edge.userp(newEdgep);
                    }
                }
            }
            પાછા newVertexp;
        }
    }

જાહેર:
    GraphAlgSubtrees(V3Graph* graphp, V3Graph* loopGraphp, V3EdgeFuncP edgeFuncp,
                     V3GraphVertex* vertexp)
        : GraphAlg<>{graphp, edgeFuncp}
        , m_loopGraphp{loopGraphp} {
        // Vertex::m_userp - New vertex if we have seen this vertex already
        // Edge::m_userp - New edge if we have seen this edge already
        m_graphp->userClearVertices();
        m_graphp->userClearEdges();
        (શૂન્ય)vertexIterateAll(vertexp);
    }
    ~GraphAlgSubtrees() = મૂળભૂત;
};

//! Report the entire connected graph with a loop or loops
શૂન્ય V3Graph::subtreeLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp, V3Graph* loopGraphp) {
    GraphAlgSubtrees{આ, loopGraphp, edgeFuncp, vertexp};
}

//######################################################################
//######################################################################
// Algorithms - sorting

શૂન્ય V3Graph::sortVertices() {
    // Sort list of vertices by rank, then fanout
    std::vector<V3GraphVertex*> vertexps;
    માટે (V3GraphVertex& vertex : m_vertices) vertexps.ધકેલો_back(&vertex);
    std::stable_sort(vertexps.begin(), vertexps.end(),
                     [](અચલ V3GraphVertex* lhsp, અચલ V3GraphVertex* rhsp) {  //
                         પાછા lhsp->sortCmp(rhsp) < 0;
                     });
    // Re-insert in sorted order
    માટે (V3GraphVertex* અચલ vertexp : vertexps) {
        m_vertices.unlink(vertexp);
        m_vertices.linkBack(vertexp);
    }
}

શૂન્ય V3Graph::sortEdges() {
    // Sort edges by rank then fanout of node they point to
    std::vector<V3GraphEdge*> edges;
    માટે (V3GraphVertex& vertex : vertices()) {
        // Make a vector
        માટે (V3GraphEdge& edge : vertex.outEdges()) edges.ધકેલો_back(&edge);
        // Sort
        std::stable_sort(edges.begin(), edges.end(),
                         [](અચલ V3GraphEdge* lhsp, અચલ V3GraphEdge* rhsp) {  //
                             પાછા lhsp->sortCmp(rhsp) < 0;
                         });
        // Relink edges in specified order
        માટે (V3GraphEdge* અચલ edgep : edges) edgep->relinkFromp(&vertex);
        // Prep for next
        edges.clear();
    }
}

//######################################################################
//######################################################################
// Algorithms - ordering
//      Compute near optimal ordering of the nodes, where:
//          If a required    edge is A->B, rank(A)<rank(B)
//          Visit edges and assign ranks to keep minimal crossings
//              (Results in better dcache packing.)

શૂન્ય V3Graph::order() {
    UINFO(2, "Order:");

    // Compute rankings again
    rank(&V3GraphEdge::followAlwaysTrue);
    orderPreRanked();
}

શૂન્ય V3Graph::orderPreRanked() {
    // Compute fanouts
    // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
    userClearVertices();
    માટે (V3GraphVertex& vertex : vertices()) {
        જો (!vertex.user()) orderDFSIterate(&vertex);
    }

    // Sort list of vertices by rank, then fanout. Fanout is a bit of a
    // misnomer. It is the sum of all the fanouts of nodes reached from a node
    // *plus* the count of edges in to that node.
    sortVertices();
    // Sort edges by rank then fanout of node they point to
    sortEdges();
}

દ્વિગુણ V3Graph::orderDFSIterate(V3GraphVertex* vertexp) {
    // Compute fanouts of each node
    // If forward edge, don't double count that fanout
    જો (vertexp->user() == 2) પાછા vertexp->fanout();  // Already processed it
    UASSERT_OBJ(vertexp->user() != 1, vertexp, "Loop found, backward edges should be dead");
    vertexp->user(1);
    દ્વિગુણ fanout = 0;
    માટે (V3GraphEdge& edge : vertexp->outEdges()) {
        જો (edge.weight()) fanout += orderDFSIterate(edge.ટોચ());
    }
    // Just count inbound edges
    માટે (V3GraphEdge& edge : vertexp->inEdges()) {
        જો (edge.weight()) ++fanout;
    }
    vertexp->fanout(fanout);
    vertexp->user(2);
    પાછા vertexp->fanout();
}

//######################################################################
//######################################################################
// Algorithms - parallelism report

વર્ગ GraphAlgParallelismReport final {
    // MEMBERS
    V3Graph& m_graph;  // The graph
    અચલ std::function<uint64_t(અચલ V3GraphVertex*)> m_vertexCost;  // vertex cost function
    V3Graph::ParallelismReport m_report;  // The result report

    // CONSTRUCTORS
    explicit GraphAlgParallelismReport(V3Graph& graph,
                                       std::function<uint64_t(અચલ V3GraphVertex*)> vertexCost)
        : m_graph{graph}
        , m_vertexCost{vertexCost} {
        // For each node, record the critical path cost from the start
        // of the graph through the end of the node.
        std::unordered_map<અચલ V3GraphVertex*, uint64_t> critPaths;
        GraphStreamUnordered serialize{&m_graph};
        માટે (અચલ V3GraphVertex* vertexp; (vertexp = serialize.nextp());) {
            ++m_report.m_vertex_ગણતરી;
            uint64_t cpCostToHere = 0;
            માટે (અચલ V3GraphEdge& edge : vertexp->inEdges()) {
                ++m_report.m_edge_ગણતરી;
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
            m_report.m_totalGraphCost += m_vertexCost(vertexp);
        }
    }
    ~GraphAlgParallelismReport() = મૂળભૂત;
    VL_UNCOPYABLE(GraphAlgParallelismReport);
    VL_UNMOVABLE(GraphAlgParallelismReport);

જાહેર:
    સ્થિર V3Graph::ParallelismReport
    apply(V3Graph& graph, std::function<uint32_t(અચલ V3GraphVertex*)> vertexCost) {
        પાછા GraphAlgParallelismReport(graph, vertexCost).m_report;
    }
};

V3Graph::ParallelismReport
V3Graph::parallelismReport(std::function<uint64_t(અચલ V3GraphVertex*)> vertexCost) {
    પાછા GraphAlgParallelismReport::apply(*આ, vertexCost);
}
