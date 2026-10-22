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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3GraphAlg.h"

#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3GraphPathChecker.h"
#अन्तर्भाव "V3GraphStream.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <limits>
#अन्तर्भाव <list>
#अन्तर्भाव <map>
#अन्तर्भाव <numeric>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - Remove redundancies
// Changes user() and weight()

वर्ग GraphRemoveRedundant final : GraphAlg<> {
    स्थिरांक तार्किक m_sumWeights;  // Sum, rather then maximize weights
खाजगी:
    शून्य main() {
        साठी (V3GraphVertex& vertex : m_graphp->vertices()) vertexIterate(&vertex);
    }
    शून्य vertexIterate(V3GraphVertex* vertexp) {
        // Clear marks
        साठी (V3GraphEdge& edge : vertexp->outEdges()) edge.शीर्ष()->userp(nullptr);

        // Mark edges and detect duplications

        साठी (V3GraphEdge* स्थिरांक edgep : vertexp->outEdges().unlinkable()) {
            जर (followEdge(edgep)) {
                V3GraphVertex* outVertexp = edgep->शीर्ष();
                V3GraphEdge* prevEdgep = static_cast<V3GraphEdge*>(outVertexp->userp());
                जर (!prevEdgep) {  // No previous assignment
                    outVertexp->userp(edgep);
                } अन्यथा {  // Duplicate
                    तार्किक saveOld = सत्य;
                    जर (prevEdgep->cutable() && !edgep->cutable()) {
                        saveOld = असत्य;  // new !cutable more important than old
                    } अन्यथा जर (!prevEdgep->cutable() && edgep->cutable()) {
                        saveOld = सत्य;  // old !cutable more important than new
                    } अन्यथा {
                        saveOld = सत्य;
                        जर (!m_sumWeights
                            && (prevEdgep->weight() < edgep->weight())) {  // Keep max weight
                            prevEdgep->weight(edgep->weight());
                        }
                    }
                    जर (saveOld) {
                        जर (m_sumWeights) prevEdgep->weight(prevEdgep->weight() + edgep->weight());
                        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    } अन्यथा {
                        जर (m_sumWeights) edgep->weight(prevEdgep->weight() + edgep->weight());
                        VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                        outVertexp->userp(edgep);
                    }
                }
            }
        }
    }

सार्वजनिक:
    GraphRemoveRedundant(V3Graph* graphp, V3EdgeFuncP edgeFuncp, तार्किक sumWeights)
        : GraphAlg<>{graphp, edgeFuncp}
        , m_sumWeights{sumWeights} {
        main();
    }
    ~GraphRemoveRedundant() = पूर्वनिर्धारित;
};

शून्य V3Graph::removeRedundantEdgesMax(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{हे, edgeFuncp, असत्य};
}
शून्य V3Graph::removeRedundantEdgesSum(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{हे, edgeFuncp, सत्य};
}

//######################################################################
//######################################################################
// Algorithms - remove transitive

वर्ग GraphAlgRemoveTransitiveEdges final : GraphAlg<> {
सार्वजनिक:
    explicit GraphAlgRemoveTransitiveEdges(V3Graph* graphp)
        : GraphAlg<>(graphp, nullptr) {}
    शून्य go() {
        GraphPathChecker checker{m_graphp};
        साठी (V3GraphVertex& vtx : m_graphp->vertices()) {
            V3GraphEdge* deletep = nullptr;
            साठी (V3GraphEdge& edge : vtx.outEdges()) {
                जर (deletep) VL_DO_CLEAR(deletep->unlinkDelete(), deletep = nullptr);
                // It should be safe to modify the graph, despite using
                // the GraphPathChecker, as none of the modifications will
                // change what can be reached from what, nor should they
                // change the rank or CP of any node.
                जर (checker.isTransitiveEdge(&edge)) deletep = &edge;
            }
            जर (deletep) VL_DO_DANGLING(deletep->unlinkDelete(), deletep);
        }
    }

खाजगी:
    VL_UNCOPYABLE(GraphAlgRemoveTransitiveEdges);
};

शून्य V3Graph::removeTransitiveEdges() { GraphAlgRemoveTransitiveEdges{हे}.go(); }

//######################################################################
//######################################################################
// Algorithms - weakly connected components
// Changes color()

वर्ग GraphAlgWeakly final : GraphAlg<> {
    // Sentinel color, meaning not colored yet. Colors themselves are 0 .. m_numColors-1.
    स्थिर constexpr uint32_t UNCOLORED = std::numeric_limits<uint32_t>::max();

    uint32_t m_अंक_Colors = 0;  // Number of colors assigned

    शून्य main() {
        // Initialize state
        साठी (V3GraphVertex& vertex : m_graphp->vertices()) vertex.color(UNCOLORED);
        // Color graph, without gaps
        साठी (V3GraphVertex& vertex : m_graphp->vertices()) {
            जर (vertex.color() == UNCOLORED) vertexIterate(&vertex, m_अंक_Colors++);
        }
    }

    शून्य vertexIterate(V3GraphVertex* vertexp, uint32_t currentColor) {
        // Assign new color to each unvisited node
        // then visit each of its edges, giving them the same color
        जर (vertexp->color() != UNCOLORED) परत;  // Already colored it
        vertexp->color(currentColor);
        साठी (V3GraphEdge& edge : vertexp->outEdges()) {
            जर (followEdge(&edge)) vertexIterate(edge.शीर्ष(), currentColor);
        }
        साठी (V3GraphEdge& edge : vertexp->inEdges()) {
            जर (followEdge(&edge)) vertexIterate(edge.fromp(), currentColor);
        }
    }

सार्वजनिक:
    GraphAlgWeakly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>(graphp, edgeFuncp) {
        main();
    }
    ~GraphAlgWeakly() = पूर्वनिर्धारित;

    uint32_t अंक_Colors() स्थिरांक { परत m_अंक_Colors; }
};

uint32_t V3Graph::weaklyConnected(V3EdgeFuncP edgeFuncp) {
    परत GraphAlgWeakly{हे, edgeFuncp}.अंक_Colors();
}

//######################################################################
//######################################################################
// Algorithms - strongly connected components
// Changes user() and color()

वर्ग GraphAlgStrongly final : GraphAlg<> {
    uint32_t m_currentDfs = 0;  // DFS count
    std::vector<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far

    शून्य main() {
        // Use Pearce's algorithm to color the strongly connected components. For reference see
        // "An Improved Algorithm for Finding the Strongly Connected Components of a Directed
        // Graph", David J.Pearce, 2005
        //
        // Node State:
        //     Vertex::user     // DFS number indicating possible root of subtree, 0=not iterated
        //     Vertex::color    // Output subtree number (fully processed)

        // Clear info
        साठी (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.color(0);
            vertex.user(0);
        }
        // Color graph
        साठी (V3GraphVertex& vertex : m_graphp->vertices()) {
            जर (!vertex.user()) {
                m_currentDfs++;
                vertexIterate(&vertex);
            }
        }
        // If there's a single vertex of a color, it doesn't need a subgraph
        // This simplifies the consumer's code, and reduces graph debugging clutter
        साठी (V3GraphVertex& vertex : m_graphp->vertices()) {
            तार्किक onecolor = सत्य;
            साठी (V3GraphEdge& edge : vertex.outEdges()) {
                जर (followEdge(&edge)) {
                    जर (vertex.color() == edge.शीर्ष()->color()) {
                        onecolor = असत्य;
                        थांबा;
                    }
                }
            }
            जर (onecolor) vertex.color(0);
        }
    }

    शून्य vertexIterate(V3GraphVertex* vertexp) {
        स्थिरांक uint32_t this_Dfs_अंक = m_currentDfs++;
        vertexp->user(this_Dfs_अंक);
        vertexp->color(0);
        साठी (V3GraphEdge& edge : vertexp->outEdges()) {
            जर (followEdge(&edge)) {
                V3GraphVertex* शीर्ष = edge.शीर्ष();
                जर (!शीर्ष->user()) {  // Dest not computed yet
                    vertexIterate(शीर्ष);
                }
                जर (!शीर्ष->color()) {  // Dest not in a component
                    जर (vertexp->user() > शीर्ष->user()) vertexp->user(शीर्ष->user());
                }
            }
        }
        जर (vertexp->user() == this_Dfs_अंक) {  // New head of subtree
            vertexp->color(this_Dfs_अंक);  // Mark as component
            पर्यंत (!m_callTrace.empty()) {
                V3GraphVertex* काढा_Vertexp = m_callTrace.back();
                जर (काढा_Vertexp->user() >= this_Dfs_अंक) {  // Lower node is part of this subtree
                    m_callTrace.काढा_back();
                    काढा_Vertexp->color(this_Dfs_अंक);
                } अन्यथा {
                    थांबा;
                }
            }
        } अन्यथा {  // In another subtree (maybe...)
            m_callTrace.ढकला_back(vertexp);
        }
    }

सार्वजनिक:
    GraphAlgStrongly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        main();
    }
    ~GraphAlgStrongly() = पूर्वनिर्धारित;
};

शून्य V3Graph::stronglyConnected(V3EdgeFuncP edgeFuncp) { GraphAlgStrongly{हे, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking
// Changes user() and rank()

वर्ग GraphAlgRank final : GraphAlg<> {
    शून्य main() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        साठी (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        साठी (V3GraphVertex& vertex : m_graphp->vertices()) {
            जर (!vertex.user()) vertexIterate(&vertex, 1);
        }
    }
    शून्य vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // If larger rank is found, assign it and loop back through
        // If we hit a back node make a list of all loops
        जर (vertexp->user() == 1) {
            m_graphp->loopsMessageCb(vertexp, m_edgeFuncp);
            परत;  // LCOV_EXCL_LINE  // gcc gprof bug misses this return
        }
        जर (vertexp->rank() >= currentRank) परत;  // Already processed it
        vertexp->user(1);
        vertexp->rank(currentRank);
        साठी (V3GraphEdge& edge : vertexp->outEdges()) {
            जर (followEdge(&edge)) vertexIterate(edge.शीर्ष(), currentRank + vertexp->rankAdder());
        }
        vertexp->user(2);
    }

सार्वजनिक:
    GraphAlgRank(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        main();
    }
    ~GraphAlgRank() = पूर्वनिर्धारित;
};

शून्य V3Graph::rank() { rank(&V3GraphEdge::followAlwaysTrue); }
शून्य V3Graph::rank(V3EdgeFuncP edgeFuncp) { GraphAlgRank{हे, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking min
// Changes user() and rank()

वर्ग GraphAlgRankMin final : GraphAlg<> {
    शून्य main() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        साठी (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        साठी (V3GraphVertex& vertex : m_graphp->vertices()) {
            जर (!vertex.user()) vertexIterate(&vertex);
        }
    }
    uint32_t vertexIterate(V3GraphVertex* vertexp) {
        // Assign rank to each unvisited node
        // If we hit a back node make a list of all loops
        जर (vertexp->user() == 1) {
            m_graphp->loopsMessageCb(vertexp, m_edgeFuncp);
            परत vertexp->rank();
        }
        जर (vertexp->user()) परत vertexp->rank();  // Done earlier
        vertexp->user(1);
        vertexp->rank(1);  // In case loop
        // If no input edges, then rank 1 (+ adder)
        // Otherwise, get minimum from following all inputs.
        uint32_t minrank = ~0U;
        साठी (V3GraphEdge& edge : vertexp->inEdges()) {
            जर (followEdge(&edge)) {
                स्थिरांक uint32_t nrank = vertexIterate(edge.fromp());
                minrank = std::min(minrank, nrank);
            }
        }
        जर (minrank == ~0U) minrank = 0;
        vertexp->rank(minrank + vertexp->rankAdder());
        vertexp->user(2);
        परत vertexp->rank();
    }

सार्वजनिक:
    GraphAlgRankMin(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        main();
    }
    ~GraphAlgRankMin() = पूर्वनिर्धारित;
};

शून्य V3Graph::rankMin() { rankMin(&V3GraphEdge::followAlwaysTrue); }
शून्य V3Graph::rankMin(V3EdgeFuncP edgeFuncp) { GraphAlgRankMin{हे, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - report loops
// Changes user()

वर्ग GraphAlgRLoops final : GraphAlg<> {
    std::vector<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far
    std::vector<string> m_msgs;  // Output messages
    तार्किक m_done = असत्य;  // Exit algorithm

    शून्य main(V3GraphVertex* vertexp) {
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        m_graphp->userClearVertices();
        m_callTrace.reserve(100);
        vertexIterate(vertexp, 0);
    }

    शून्य vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // When we hit ourself again, return the list of all loops
        जर (m_done) परत;

        // Can't just reserve(), unless we modify size() before setting array directly
        पर्यंत (m_callTrace.size() <= currentRank) m_callTrace.ढकला_back(vertexp);
        m_callTrace[currentRank++] = vertexp;

        जर (vertexp->user() == 1) {
            साठी (अचिन्हित i = 0; i < currentRank; i++)
                m_msgs.emplace_back(m_graphp->loopsVertexCb(m_callTrace[i]));
            m_done = सत्य;
            परत;
        }
        जर (vertexp->user() == 2) परत;  // Already processed it
        vertexp->user(1);
        साठी (V3GraphEdge& edge : vertexp->outEdges()) {
            जर (followEdge(&edge)) vertexIterate(edge.शीर्ष(), currentRank);
        }
        vertexp->user(2);
    }

सार्वजनिक:
    GraphAlgRLoops(V3Graph* graphp, V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp)
        : GraphAlg<>{graphp, edgeFuncp} {
        main(vertexp);
    }
    ~GraphAlgRLoops() = पूर्वनिर्धारित;
    string message() स्थिरांक {
        परत std::accumulate(m_msgs.begin(), m_msgs.end(), std::string{""});
    }
};

string V3Graph::reportLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp) {
    परत GraphAlgRLoops{हे, edgeFuncp, vertexp}.message();
}

//######################################################################
//######################################################################
// Algorithms - subtrees
// Changes user()

वर्ग GraphAlgSubtrees final : GraphAlg<> {
    V3Graph* स्थिरांक m_loopGraphp;

    //! Iterate through all connected nodes of a graph with a loop or loops.
    V3GraphVertex* vertexIterateAll(V3GraphVertex* vertexp) {
        जर (V3GraphVertex* newVertexp = static_cast<V3GraphVertex*>(vertexp->userp())) {
            परत newVertexp;
        } अन्यथा {
            newVertexp = vertexp->clone(m_loopGraphp);
            vertexp->userp(newVertexp);

            साठी (V3GraphEdge& edge : vertexp->outEdges()) {
                जर (followEdge(&edge)) {
                    V3GraphEdge* newEdgep = static_cast<V3GraphEdge*>(edge.userp());
                    जर (!newEdgep) {
                        V3GraphVertex* new_शीर्ष = vertexIterateAll(edge.शीर्ष());
                        newEdgep = edge.clone(m_loopGraphp, newVertexp, new_शीर्ष);
                        edge.userp(newEdgep);
                    }
                }
            }
            परत newVertexp;
        }
    }

सार्वजनिक:
    GraphAlgSubtrees(V3Graph* graphp, V3Graph* loopGraphp, V3EdgeFuncP edgeFuncp,
                     V3GraphVertex* vertexp)
        : GraphAlg<>{graphp, edgeFuncp}
        , m_loopGraphp{loopGraphp} {
        // Vertex::m_userp - New vertex if we have seen this vertex already
        // Edge::m_userp - New edge if we have seen this edge already
        m_graphp->userClearVertices();
        m_graphp->userClearEdges();
        (शून्य)vertexIterateAll(vertexp);
    }
    ~GraphAlgSubtrees() = पूर्वनिर्धारित;
};

//! Report the entire connected graph with a loop or loops
शून्य V3Graph::subtreeLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp, V3Graph* loopGraphp) {
    GraphAlgSubtrees{हे, loopGraphp, edgeFuncp, vertexp};
}

//######################################################################
//######################################################################
// Algorithms - sorting

शून्य V3Graph::sortVertices() {
    // Sort list of vertices by rank, then fanout
    std::vector<V3GraphVertex*> vertexps;
    साठी (V3GraphVertex& vertex : m_vertices) vertexps.ढकला_back(&vertex);
    std::stable_sort(vertexps.begin(), vertexps.end(),
                     [](स्थिरांक V3GraphVertex* lhsp, स्थिरांक V3GraphVertex* rhsp) {  //
                         परत lhsp->sortCmp(rhsp) < 0;
                     });
    // Re-insert in sorted order
    साठी (V3GraphVertex* स्थिरांक vertexp : vertexps) {
        m_vertices.unlink(vertexp);
        m_vertices.linkBack(vertexp);
    }
}

शून्य V3Graph::sortEdges() {
    // Sort edges by rank then fanout of node they point to
    std::vector<V3GraphEdge*> edges;
    साठी (V3GraphVertex& vertex : vertices()) {
        // Make a vector
        साठी (V3GraphEdge& edge : vertex.outEdges()) edges.ढकला_back(&edge);
        // Sort
        std::stable_sort(edges.begin(), edges.end(),
                         [](स्थिरांक V3GraphEdge* lhsp, स्थिरांक V3GraphEdge* rhsp) {  //
                             परत lhsp->sortCmp(rhsp) < 0;
                         });
        // Relink edges in specified order
        साठी (V3GraphEdge* स्थिरांक edgep : edges) edgep->relinkFromp(&vertex);
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

शून्य V3Graph::order() {
    UINFO(2, "Order:");

    // Compute rankings again
    rank(&V3GraphEdge::followAlwaysTrue);
    orderPreRanked();
}

शून्य V3Graph::orderPreRanked() {
    // Compute fanouts
    // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
    userClearVertices();
    साठी (V3GraphVertex& vertex : vertices()) {
        जर (!vertex.user()) orderDFSIterate(&vertex);
    }

    // Sort list of vertices by rank, then fanout. Fanout is a bit of a
    // misnomer. It is the sum of all the fanouts of nodes reached from a node
    // *plus* the count of edges in to that node.
    sortVertices();
    // Sort edges by rank then fanout of node they point to
    sortEdges();
}

द्विगुण V3Graph::orderDFSIterate(V3GraphVertex* vertexp) {
    // Compute fanouts of each node
    // If forward edge, don't double count that fanout
    जर (vertexp->user() == 2) परत vertexp->fanout();  // Already processed it
    UASSERT_OBJ(vertexp->user() != 1, vertexp, "Loop found, backward edges should be dead");
    vertexp->user(1);
    द्विगुण fanout = 0;
    साठी (V3GraphEdge& edge : vertexp->outEdges()) {
        जर (edge.weight()) fanout += orderDFSIterate(edge.शीर्ष());
    }
    // Just count inbound edges
    साठी (V3GraphEdge& edge : vertexp->inEdges()) {
        जर (edge.weight()) ++fanout;
    }
    vertexp->fanout(fanout);
    vertexp->user(2);
    परत vertexp->fanout();
}

//######################################################################
//######################################################################
// Algorithms - parallelism report

वर्ग GraphAlgParallelismReport final {
    // MEMBERS
    V3Graph& m_graph;  // The graph
    स्थिरांक std::function<uint64_t(स्थिरांक V3GraphVertex*)> m_vertexCost;  // vertex cost function
    V3Graph::ParallelismReport m_report;  // The result report

    // CONSTRUCTORS
    explicit GraphAlgParallelismReport(V3Graph& graph,
                                       std::function<uint64_t(स्थिरांक V3GraphVertex*)> vertexCost)
        : m_graph{graph}
        , m_vertexCost{vertexCost} {
        // For each node, record the critical path cost from the start
        // of the graph through the end of the node.
        std::unordered_map<स्थिरांक V3GraphVertex*, uint64_t> critPaths;
        GraphStreamUnordered serialize{&m_graph};
        साठी (स्थिरांक V3GraphVertex* vertexp; (vertexp = serialize.nextp());) {
            ++m_report.m_vertex_मोजणी;
            uint64_t cpCostToHere = 0;
            साठी (स्थिरांक V3GraphEdge& edge : vertexp->inEdges()) {
                ++m_report.m_edge_मोजणी;
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
    ~GraphAlgParallelismReport() = पूर्वनिर्धारित;
    VL_UNCOPYABLE(GraphAlgParallelismReport);
    VL_UNMOVABLE(GraphAlgParallelismReport);

सार्वजनिक:
    स्थिर V3Graph::ParallelismReport
    apply(V3Graph& graph, std::function<uint32_t(स्थिरांक V3GraphVertex*)> vertexCost) {
        परत GraphAlgParallelismReport(graph, vertexCost).m_report;
    }
};

V3Graph::ParallelismReport
V3Graph::parallelismReport(std::function<uint64_t(स्थिरांक V3GraphVertex*)> vertexCost) {
    परत GraphAlgParallelismReport::apply(*हे, vertexCost);
}
