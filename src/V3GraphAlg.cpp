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

#include "config_build.h"
#include "verilatedos.h"

#include "V3GraphAlg.h"

#include "V3Global.h"
#include "V3GraphPathChecker.h"
#include "V3GraphStream.h"
#include "V3Stats.h"

#include <algorithm>
#include <limits>
#include <list>
#include <map>
#include <numeric>
#include <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - Remove redundancies
// Changes user() and weight()

class GraphRemoveRedundant final : GraphAlg<> {
    const bool m_sumWeights;  // Sum, rather then maximize weights
private:
    void main() {
        དོན་ལུ (V3GraphVertex& vertex : m_graphp->vertices()) vertexIterate(&vertex);
    }
    void vertexIterate(V3GraphVertex* vertexp) {
        // Clear marks
        དོན་ལུ (V3GraphEdge& edge : vertexp->outEdges()) edge.top()->userp(nullptr);

        // Mark edges and detect duplications

        དོན་ལུ (V3GraphEdge* const edgep : vertexp->outEdges().unlinkable()) {
            གལ་སྲིད (followEdge(edgep)) {
                V3GraphVertex* outVertexp = edgep->top();
                V3GraphEdge* prevEdgep = static_cast<V3GraphEdge*>(outVertexp->userp());
                གལ་སྲིད (!prevEdgep) {  // No previous assignment
                    outVertexp->userp(edgep);
                } དེ་མིན {  // Duplicate
                    bool saveOld = true;
                    གལ་སྲིད (prevEdgep->cutable() && !edgep->cutable()) {
                        saveOld = false;  // new !cutable more important than old
                    } དེ་མིན གལ་སྲིད (!prevEdgep->cutable() && edgep->cutable()) {
                        saveOld = true;  // old !cutable more important than new
                    } དེ་མིན {
                        saveOld = true;
                        གལ་སྲིད (!m_sumWeights
                            && (prevEdgep->weight() < edgep->weight())) {  // Keep max weight
                            prevEdgep->weight(edgep->weight());
                        }
                    }
                    གལ་སྲིད (saveOld) {
                        གལ་སྲིད (m_sumWeights) prevEdgep->weight(prevEdgep->weight() + edgep->weight());
                        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    } དེ་མིན {
                        གལ་སྲིད (m_sumWeights) edgep->weight(prevEdgep->weight() + edgep->weight());
                        VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                        outVertexp->userp(edgep);
                    }
                }
            }
        }
    }

public:
    GraphRemoveRedundant(V3Graph* graphp, V3EdgeFuncP edgeFuncp, bool sumWeights)
        : GraphAlg<>{graphp, edgeFuncp}
        , m_sumWeights{sumWeights} {
        main();
    }
    ~GraphRemoveRedundant() = default;
};

void V3Graph::removeRedundantEdgesMax(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{this, edgeFuncp, false};
}
void V3Graph::removeRedundantEdgesSum(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{this, edgeFuncp, true};
}

//######################################################################
//######################################################################
// Algorithms - remove transitive

class GraphAlgRemoveTransitiveEdges final : GraphAlg<> {
public:
    explicit GraphAlgRemoveTransitiveEdges(V3Graph* graphp)
        : GraphAlg<>(graphp, nullptr) {}
    void go() {
        GraphPathChecker checker{m_graphp};
        དོན་ལུ (V3GraphVertex& vtx : m_graphp->vertices()) {
            V3GraphEdge* deletep = nullptr;
            དོན་ལུ (V3GraphEdge& edge : vtx.outEdges()) {
                གལ་སྲིད (deletep) VL_DO_CLEAR(deletep->unlinkDelete(), deletep = nullptr);
                // It should be safe to modify the graph, despite using
                // the GraphPathChecker, as none of the modifications will
                // change what can be reached from what, nor should they
                // change the rank or CP of any node.
                གལ་སྲིད (checker.isTransitiveEdge(&edge)) deletep = &edge;
            }
            གལ་སྲིད (deletep) VL_DO_DANGLING(deletep->unlinkDelete(), deletep);
        }
    }

private:
    VL_UNCOPYABLE(GraphAlgRemoveTransitiveEdges);
};

void V3Graph::removeTransitiveEdges() { GraphAlgRemoveTransitiveEdges{this}.go(); }

//######################################################################
//######################################################################
// Algorithms - weakly connected components
// Changes color()

class GraphAlgWeakly final : GraphAlg<> {
    // Sentinel color, meaning not colored yet. Colors themselves are 0 .. m_numColors-1.
    static constexpr uint32_t UNCOLORED = std::numeric_limits<uint32_t>::max();

    uint32_t m_numColors = 0;  // Number of colors assigned

    void main() {
        // Initialize state
        དོན་ལུ (V3GraphVertex& vertex : m_graphp->vertices()) vertex.color(UNCOLORED);
        // Color graph, without gaps
        དོན་ལུ (V3GraphVertex& vertex : m_graphp->vertices()) {
            གལ་སྲིད (vertex.color() == UNCOLORED) vertexIterate(&vertex, m_numColors++);
        }
    }

    void vertexIterate(V3GraphVertex* vertexp, uint32_t currentColor) {
        // Assign new color to each unvisited node
        // then visit each of its edges, giving them the same color
        གལ་སྲིད (vertexp->color() != UNCOLORED) ལོག;  // Already colored it
        vertexp->color(currentColor);
        དོན་ལུ (V3GraphEdge& edge : vertexp->outEdges()) {
            གལ་སྲིད (followEdge(&edge)) vertexIterate(edge.top(), currentColor);
        }
        དོན་ལུ (V3GraphEdge& edge : vertexp->inEdges()) {
            གལ་སྲིད (followEdge(&edge)) vertexIterate(edge.fromp(), currentColor);
        }
    }

public:
    GraphAlgWeakly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>(graphp, edgeFuncp) {
        main();
    }
    ~GraphAlgWeakly() = default;

    uint32_t numColors() const { ལོག m_numColors; }
};

uint32_t V3Graph::weaklyConnected(V3EdgeFuncP edgeFuncp) {
    ལོག GraphAlgWeakly{this, edgeFuncp}.numColors();
}

//######################################################################
//######################################################################
// Algorithms - strongly connected components
// Changes user() and color()

class GraphAlgStrongly final : GraphAlg<> {
    uint32_t m_currentDfs = 0;  // DFS count
    std::vector<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far

    void main() {
        // Use Pearce's algorithm to color the strongly connected components. For reference see
        // "An Improved Algorithm for Finding the Strongly Connected Components of a Directed
        // Graph", David J.Pearce, 2005
        //
        // Node State:
        //     Vertex::user     // DFS number indicating possible root of subtree, 0=not iterated
        //     Vertex::color    // Output subtree number (fully processed)

        // Clear info
        དོན་ལུ (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.color(0);
            vertex.user(0);
        }
        // Color graph
        དོན་ལུ (V3GraphVertex& vertex : m_graphp->vertices()) {
            གལ་སྲིད (!vertex.user()) {
                m_currentDfs++;
                vertexIterate(&vertex);
            }
        }
        // If there's a single vertex of a color, it doesn't need a subgraph
        // This simplifies the consumer's code, and reduces graph debugging clutter
        དོན་ལུ (V3GraphVertex& vertex : m_graphp->vertices()) {
            bool onecolor = true;
            དོན་ལུ (V3GraphEdge& edge : vertex.outEdges()) {
                གལ་སྲིད (followEdge(&edge)) {
                    གལ་སྲིད (vertex.color() == edge.top()->color()) {
                        onecolor = false;
                        bha_nga;
                    }
                }
            }
            གལ་སྲིད (onecolor) vertex.color(0);
        }
    }

    void vertexIterate(V3GraphVertex* vertexp) {
        const uint32_t thisDfsNum = m_currentDfs++;
        vertexp->user(thisDfsNum);
        vertexp->color(0);
        དོན་ལུ (V3GraphEdge& edge : vertexp->outEdges()) {
            གལ་སྲིད (followEdge(&edge)) {
                V3GraphVertex* top = edge.top();
                གལ་སྲིད (!top->user()) {  // Dest not computed yet
                    vertexIterate(top);
                }
                གལ་སྲིད (!top->color()) {  // Dest not in a component
                    གལ་སྲིད (vertexp->user() > top->user()) vertexp->user(top->user());
                }
            }
        }
        གལ་སྲིད (vertexp->user() == thisDfsNum) {  // New head of subtree
            vertexp->color(thisDfsNum);  // Mark as component
            སྐབས (!m_callTrace.empty()) {
                V3GraphVertex* popVertexp = m_callTrace.back();
                གལ་སྲིད (popVertexp->user() >= thisDfsNum) {  // Lower node is part of this subtree
                    m_callTrace.pop_back();
                    popVertexp->color(thisDfsNum);
                } དེ་མིན {
                    bha_nga;
                }
            }
        } དེ་མིན {  // In another subtree (maybe...)
            m_callTrace.push_back(vertexp);
        }
    }

public:
    GraphAlgStrongly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        main();
    }
    ~GraphAlgStrongly() = default;
};

void V3Graph::stronglyConnected(V3EdgeFuncP edgeFuncp) { GraphAlgStrongly{this, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking
// Changes user() and rank()

class GraphAlgRank final : GraphAlg<> {
    void main() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        དོན་ལུ (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        དོན་ལུ (V3GraphVertex& vertex : m_graphp->vertices()) {
            གལ་སྲིད (!vertex.user()) vertexIterate(&vertex, 1);
        }
    }
    void vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // If larger rank is found, assign it and loop back through
        // If we hit a back node make a list of all loops
        གལ་སྲིད (vertexp->user() == 1) {
            m_graphp->loopsMessageCb(vertexp, m_edgeFuncp);
            ལོག;  // LCOV_EXCL_LINE  // gcc gprof bug misses this return
        }
        གལ་སྲིད (vertexp->rank() >= currentRank) ལོག;  // Already processed it
        vertexp->user(1);
        vertexp->rank(currentRank);
        དོན་ལུ (V3GraphEdge& edge : vertexp->outEdges()) {
            གལ་སྲིད (followEdge(&edge)) vertexIterate(edge.top(), currentRank + vertexp->rankAdder());
        }
        vertexp->user(2);
    }

public:
    GraphAlgRank(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        main();
    }
    ~GraphAlgRank() = default;
};

void V3Graph::rank() { rank(&V3GraphEdge::followAlwaysTrue); }
void V3Graph::rank(V3EdgeFuncP edgeFuncp) { GraphAlgRank{this, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking min
// Changes user() and rank()

class GraphAlgRankMin final : GraphAlg<> {
    void main() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        དོན་ལུ (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        དོན་ལུ (V3GraphVertex& vertex : m_graphp->vertices()) {
            གལ་སྲིད (!vertex.user()) vertexIterate(&vertex);
        }
    }
    uint32_t vertexIterate(V3GraphVertex* vertexp) {
        // Assign rank to each unvisited node
        // If we hit a back node make a list of all loops
        གལ་སྲིད (vertexp->user() == 1) {
            m_graphp->loopsMessageCb(vertexp, m_edgeFuncp);
            ལོག vertexp->rank();
        }
        གལ་སྲིད (vertexp->user()) ལོག vertexp->rank();  // Done earlier
        vertexp->user(1);
        vertexp->rank(1);  // In case loop
        // If no input edges, then rank 1 (+ adder)
        // Otherwise, get minimum from following all inputs.
        uint32_t minrank = ~0U;
        དོན་ལུ (V3GraphEdge& edge : vertexp->inEdges()) {
            གལ་སྲིད (followEdge(&edge)) {
                const uint32_t nrank = vertexIterate(edge.fromp());
                minrank = std::min(minrank, nrank);
            }
        }
        གལ་སྲིད (minrank == ~0U) minrank = 0;
        vertexp->rank(minrank + vertexp->rankAdder());
        vertexp->user(2);
        ལོག vertexp->rank();
    }

public:
    GraphAlgRankMin(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        main();
    }
    ~GraphAlgRankMin() = default;
};

void V3Graph::rankMin() { rankMin(&V3GraphEdge::followAlwaysTrue); }
void V3Graph::rankMin(V3EdgeFuncP edgeFuncp) { GraphAlgRankMin{this, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - report loops
// Changes user()

class GraphAlgRLoops final : GraphAlg<> {
    std::vector<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far
    std::vector<string> m_msgs;  // Output messages
    bool m_done = false;  // Exit algorithm

    void main(V3GraphVertex* vertexp) {
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        m_graphp->userClearVertices();
        m_callTrace.reserve(100);
        vertexIterate(vertexp, 0);
    }

    void vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // When we hit ourself again, return the list of all loops
        གལ་སྲིད (m_done) ལོག;

        // Can't just reserve(), unless we modify size() before setting array directly
        སྐབས (m_callTrace.size() <= currentRank) m_callTrace.push_back(vertexp);
        m_callTrace[currentRank++] = vertexp;

        གལ་སྲིད (vertexp->user() == 1) {
            དོན་ལུ (unsigned i = 0; i < currentRank; i++)
                m_msgs.emplace_back(m_graphp->loopsVertexCb(m_callTrace[i]));
            m_done = true;
            ལོག;
        }
        གལ་སྲིད (vertexp->user() == 2) ལོག;  // Already processed it
        vertexp->user(1);
        དོན་ལུ (V3GraphEdge& edge : vertexp->outEdges()) {
            གལ་སྲིད (followEdge(&edge)) vertexIterate(edge.top(), currentRank);
        }
        vertexp->user(2);
    }

public:
    GraphAlgRLoops(V3Graph* graphp, V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp)
        : GraphAlg<>{graphp, edgeFuncp} {
        main(vertexp);
    }
    ~GraphAlgRLoops() = default;
    string message() const {
        ལོག std::accumulate(m_msgs.begin(), m_msgs.end(), std::string{""});
    }
};

string V3Graph::reportLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp) {
    ལོག GraphAlgRLoops{this, edgeFuncp, vertexp}.message();
}

//######################################################################
//######################################################################
// Algorithms - subtrees
// Changes user()

class GraphAlgSubtrees final : GraphAlg<> {
    V3Graph* const m_loopGraphp;

    //! Iterate through all connected nodes of a graph with a loop or loops.
    V3GraphVertex* vertexIterateAll(V3GraphVertex* vertexp) {
        གལ་སྲིད (V3GraphVertex* newVertexp = static_cast<V3GraphVertex*>(vertexp->userp())) {
            ལོག newVertexp;
        } དེ་མིན {
            newVertexp = vertexp->clone(m_loopGraphp);
            vertexp->userp(newVertexp);

            དོན་ལུ (V3GraphEdge& edge : vertexp->outEdges()) {
                གལ་སྲིད (followEdge(&edge)) {
                    V3GraphEdge* newEdgep = static_cast<V3GraphEdge*>(edge.userp());
                    གལ་སྲིད (!newEdgep) {
                        V3GraphVertex* newTop = vertexIterateAll(edge.top());
                        newEdgep = edge.clone(m_loopGraphp, newVertexp, newTop);
                        edge.userp(newEdgep);
                    }
                }
            }
            ལོག newVertexp;
        }
    }

public:
    GraphAlgSubtrees(V3Graph* graphp, V3Graph* loopGraphp, V3EdgeFuncP edgeFuncp,
                     V3GraphVertex* vertexp)
        : GraphAlg<>{graphp, edgeFuncp}
        , m_loopGraphp{loopGraphp} {
        // Vertex::m_userp - New vertex if we have seen this vertex already
        // Edge::m_userp - New edge if we have seen this edge already
        m_graphp->userClearVertices();
        m_graphp->userClearEdges();
        (void)vertexIterateAll(vertexp);
    }
    ~GraphAlgSubtrees() = default;
};

//! Report the entire connected graph with a loop or loops
void V3Graph::subtreeLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp, V3Graph* loopGraphp) {
    GraphAlgSubtrees{this, loopGraphp, edgeFuncp, vertexp};
}

//######################################################################
//######################################################################
// Algorithms - sorting

void V3Graph::sortVertices() {
    // Sort list of vertices by rank, then fanout
    std::vector<V3GraphVertex*> vertexps;
    དོན་ལུ (V3GraphVertex& vertex : m_vertices) vertexps.push_back(&vertex);
    std::stable_sort(vertexps.begin(), vertexps.end(),
                     [](const V3GraphVertex* lhsp, const V3GraphVertex* rhsp) {  //
                         ལོག lhsp->sortCmp(rhsp) < 0;
                     });
    // Re-insert in sorted order
    དོན་ལུ (V3GraphVertex* const vertexp : vertexps) {
        m_vertices.unlink(vertexp);
        m_vertices.linkBack(vertexp);
    }
}

void V3Graph::sortEdges() {
    // Sort edges by rank then fanout of node they point to
    std::vector<V3GraphEdge*> edges;
    དོན་ལུ (V3GraphVertex& vertex : vertices()) {
        // Make a vector
        དོན་ལུ (V3GraphEdge& edge : vertex.outEdges()) edges.push_back(&edge);
        // Sort
        std::stable_sort(edges.begin(), edges.end(),
                         [](const V3GraphEdge* lhsp, const V3GraphEdge* rhsp) {  //
                             ལོག lhsp->sortCmp(rhsp) < 0;
                         });
        // Relink edges in specified order
        དོན་ལུ (V3GraphEdge* const edgep : edges) edgep->relinkFromp(&vertex);
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

void V3Graph::order() {
    UINFO(2, "Order:");

    // Compute rankings again
    rank(&V3GraphEdge::followAlwaysTrue);
    orderPreRanked();
}

void V3Graph::orderPreRanked() {
    // Compute fanouts
    // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
    userClearVertices();
    དོན་ལུ (V3GraphVertex& vertex : vertices()) {
        གལ་སྲིད (!vertex.user()) orderDFSIterate(&vertex);
    }

    // Sort list of vertices by rank, then fanout. Fanout is a bit of a
    // misnomer. It is the sum of all the fanouts of nodes reached from a node
    // *plus* the count of edges in to that node.
    sortVertices();
    // Sort edges by rank then fanout of node they point to
    sortEdges();
}

dviguu_na V3Graph::orderDFSIterate(V3GraphVertex* vertexp) {
    // Compute fanouts of each node
    // If forward edge, don't double count that fanout
    གལ་སྲིད (vertexp->user() == 2) ལོག vertexp->fanout();  // Already processed it
    UASSERT_OBJ(vertexp->user() != 1, vertexp, "Loop found, backward edges should be dead");
    vertexp->user(1);
    dviguu_na fanout = 0;
    དོན་ལུ (V3GraphEdge& edge : vertexp->outEdges()) {
        གལ་སྲིད (edge.weight()) fanout += orderDFSIterate(edge.top());
    }
    // Just count inbound edges
    དོན་ལུ (V3GraphEdge& edge : vertexp->inEdges()) {
        གལ་སྲིད (edge.weight()) ++fanout;
    }
    vertexp->fanout(fanout);
    vertexp->user(2);
    ལོག vertexp->fanout();
}

//######################################################################
//######################################################################
// Algorithms - parallelism report

class GraphAlgParallelismReport final {
    // MEMBERS
    V3Graph& m_graph;  // The graph
    const std::function<uint64_t(const V3GraphVertex*)> m_vertexCost;  // vertex cost function
    V3Graph::ParallelismReport m_report;  // The result report

    // CONSTRUCTORS
    explicit GraphAlgParallelismReport(V3Graph& graph,
                                       std::function<uint64_t(const V3GraphVertex*)> vertexCost)
        : m_graph{graph}
        , m_vertexCost{vertexCost} {
        // For each node, record the critical path cost from the start
        // of the graph through the end of the node.
        std::unordered_map<const V3GraphVertex*, uint64_t> critPaths;
        GraphStreamUnordered serialize{&m_graph};
        དོན་ལུ (const V3GraphVertex* vertexp; (vertexp = serialize.nextp());) {
            ++m_report.m_vertexCount;
            uint64_t cpCostToHere = 0;
            དོན་ལུ (const V3GraphEdge& edge : vertexp->inEdges()) {
                ++m_report.m_edgeCount;
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
    ~GraphAlgParallelismReport() = default;
    VL_UNCOPYABLE(GraphAlgParallelismReport);
    VL_UNMOVABLE(GraphAlgParallelismReport);

public:
    static V3Graph::ParallelismReport
    apply(V3Graph& graph, std::function<uint32_t(const V3GraphVertex*)> vertexCost) {
        ལོག GraphAlgParallelismReport(graph, vertexCost).m_report;
    }
};

V3Graph::ParallelismReport
V3Graph::parallelismReport(std::function<uint64_t(const V3GraphVertex*)> vertexCost) {
    ལོག GraphAlgParallelismReport::apply(*this, vertexCost);
}
