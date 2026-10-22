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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3GraphAlg.h"

#شامل "V3Global.h"
#شامل "V3GraphPathChecker.h"
#شامل "V3GraphStream.h"
#شامل "V3Stats.h"

#شامل <algorithm>
#شامل <limits>
#شامل <list>
#شامل <map>
#شامل <numeric>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - Remove redundancies
// Changes user() and weight()

رده GraphRemoveRedundant final : GraphAlg<> {
    ثابت منطقی m_sumWeights;  // Sum, rather then maximize weights
خصوصی:
    تهی اصلی() {
        برای (V3GraphVertex& vertex : m_graphp->vertices()) vertexIterate(&vertex);
    }
    تهی vertexIterate(V3GraphVertex* vertexp) {
        // Clear marks
        برای (V3GraphEdge& edge : vertexp->outEdges()) edge.بالاترین()->userp(nullptr);

        // Mark edges and detect duplications

        برای (V3GraphEdge* ثابت edgep : vertexp->outEdges().unlinkable()) {
            اگر (followEdge(edgep)) {
                V3GraphVertex* outVertexp = edgep->بالاترین();
                V3GraphEdge* prevEdgep = static_cast<V3GraphEdge*>(outVertexp->userp());
                اگر (!prevEdgep) {  // No previous assignment
                    outVertexp->userp(edgep);
                } وگرنه {  // Duplicate
                    منطقی saveOld = درست;
                    اگر (prevEdgep->cutable() && !edgep->cutable()) {
                        saveOld = نادرست;  // new !cutable more important than old
                    } وگرنه اگر (!prevEdgep->cutable() && edgep->cutable()) {
                        saveOld = درست;  // old !cutable more important than new
                    } وگرنه {
                        saveOld = درست;
                        اگر (!m_sumWeights
                            && (prevEdgep->weight() < edgep->weight())) {  // Keep max weight
                            prevEdgep->weight(edgep->weight());
                        }
                    }
                    اگر (saveOld) {
                        اگر (m_sumWeights) prevEdgep->weight(prevEdgep->weight() + edgep->weight());
                        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    } وگرنه {
                        اگر (m_sumWeights) edgep->weight(prevEdgep->weight() + edgep->weight());
                        VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                        outVertexp->userp(edgep);
                    }
                }
            }
        }
    }

عمومی:
    GraphRemoveRedundant(V3Graph* graphp, V3EdgeFuncP edgeFuncp, منطقی sumWeights)
        : GraphAlg<>{graphp, edgeFuncp}
        , m_sumWeights{sumWeights} {
        اصلی();
    }
    ~GraphRemoveRedundant() = پیشفرض;
};

تهی V3Graph::removeRedundantEdgesMax(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{این, edgeFuncp, نادرست};
}
تهی V3Graph::removeRedundantEdgesSum(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{این, edgeFuncp, درست};
}

//######################################################################
//######################################################################
// Algorithms - remove transitive

رده GraphAlgRemoveTransitiveEdges final : GraphAlg<> {
عمومی:
    explicit GraphAlgRemoveTransitiveEdges(V3Graph* graphp)
        : GraphAlg<>(graphp, nullptr) {}
    تهی go() {
        GraphPathChecker checker{m_graphp};
        برای (V3GraphVertex& vtx : m_graphp->vertices()) {
            V3GraphEdge* deletep = nullptr;
            برای (V3GraphEdge& edge : vtx.outEdges()) {
                اگر (deletep) VL_DO_CLEAR(deletep->unlinkDelete(), deletep = nullptr);
                // It should be safe to modify the graph, despite using
                // the GraphPathChecker, as none of the modifications will
                // change what can be reached from what, nor should they
                // change the rank or CP of any node.
                اگر (checker.isTransitiveEdge(&edge)) deletep = &edge;
            }
            اگر (deletep) VL_DO_DANGLING(deletep->unlinkDelete(), deletep);
        }
    }

خصوصی:
    VL_UNCOPYABLE(GraphAlgRemoveTransitiveEdges);
};

تهی V3Graph::removeTransitiveEdges() { GraphAlgRemoveTransitiveEdges{این}.go(); }

//######################################################################
//######################################################################
// Algorithms - weakly connected components
// Changes color()

رده GraphAlgWeakly final : GraphAlg<> {
    // Sentinel color, meaning not colored yet. Colors themselves are 0 .. m_numColors-1.
    ایستا constexpr uint32_t UNCOLORED = std::numeric_limits<uint32_t>::max();

    uint32_t m_رقم_Colors = 0;  // Number of colors assigned

    تهی اصلی() {
        // Initialize state
        برای (V3GraphVertex& vertex : m_graphp->vertices()) vertex.color(UNCOLORED);
        // Color graph, without gaps
        برای (V3GraphVertex& vertex : m_graphp->vertices()) {
            اگر (vertex.color() == UNCOLORED) vertexIterate(&vertex, m_رقم_Colors++);
        }
    }

    تهی vertexIterate(V3GraphVertex* vertexp, uint32_t currentColor) {
        // Assign new color to each unvisited node
        // then visit each of its edges, giving them the same color
        اگر (vertexp->color() != UNCOLORED) بازگشت;  // Already colored it
        vertexp->color(currentColor);
        برای (V3GraphEdge& edge : vertexp->outEdges()) {
            اگر (followEdge(&edge)) vertexIterate(edge.بالاترین(), currentColor);
        }
        برای (V3GraphEdge& edge : vertexp->inEdges()) {
            اگر (followEdge(&edge)) vertexIterate(edge.fromp(), currentColor);
        }
    }

عمومی:
    GraphAlgWeakly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>(graphp, edgeFuncp) {
        اصلی();
    }
    ~GraphAlgWeakly() = پیشفرض;

    uint32_t رقم_Colors() ثابت { بازگشت m_رقم_Colors; }
};

uint32_t V3Graph::weaklyConnected(V3EdgeFuncP edgeFuncp) {
    بازگشت GraphAlgWeakly{این, edgeFuncp}.رقم_Colors();
}

//######################################################################
//######################################################################
// Algorithms - strongly connected components
// Changes user() and color()

رده GraphAlgStrongly final : GraphAlg<> {
    uint32_t m_currentDfs = 0;  // DFS count
    std::vector<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far

    تهی اصلی() {
        // Use Pearce's algorithm to color the strongly connected components. For reference see
        // "An Improved Algorithm for Finding the Strongly Connected Components of a Directed
        // Graph", David J.Pearce, 2005
        //
        // Node State:
        //     Vertex::user     // DFS number indicating possible root of subtree, 0=not iterated
        //     Vertex::color    // Output subtree number (fully processed)

        // Clear info
        برای (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.color(0);
            vertex.user(0);
        }
        // Color graph
        برای (V3GraphVertex& vertex : m_graphp->vertices()) {
            اگر (!vertex.user()) {
                m_currentDfs++;
                vertexIterate(&vertex);
            }
        }
        // If there's a single vertex of a color, it doesn't need a subgraph
        // This simplifies the consumer's code, and reduces graph debugging clutter
        برای (V3GraphVertex& vertex : m_graphp->vertices()) {
            منطقی onecolor = درست;
            برای (V3GraphEdge& edge : vertex.outEdges()) {
                اگر (followEdge(&edge)) {
                    اگر (vertex.color() == edge.بالاترین()->color()) {
                        onecolor = نادرست;
                        بشکن;
                    }
                }
            }
            اگر (onecolor) vertex.color(0);
        }
    }

    تهی vertexIterate(V3GraphVertex* vertexp) {
        ثابت uint32_t this_Dfs_رقم = m_currentDfs++;
        vertexp->user(this_Dfs_رقم);
        vertexp->color(0);
        برای (V3GraphEdge& edge : vertexp->outEdges()) {
            اگر (followEdge(&edge)) {
                V3GraphVertex* بالاترین = edge.بالاترین();
                اگر (!بالاترین->user()) {  // Dest not computed yet
                    vertexIterate(بالاترین);
                }
                اگر (!بالاترین->color()) {  // Dest not in a component
                    اگر (vertexp->user() > بالاترین->user()) vertexp->user(بالاترین->user());
                }
            }
        }
        اگر (vertexp->user() == this_Dfs_رقم) {  // New head of subtree
            vertexp->color(this_Dfs_رقم);  // Mark as component
            وقتی (!m_callTrace.empty()) {
                V3GraphVertex* برداشت_Vertexp = m_callTrace.back();
                اگر (برداشت_Vertexp->user() >= this_Dfs_رقم) {  // Lower node is part of this subtree
                    m_callTrace.برداشت_back();
                    برداشت_Vertexp->color(this_Dfs_رقم);
                } وگرنه {
                    بشکن;
                }
            }
        } وگرنه {  // In another subtree (maybe...)
            m_callTrace.افزودن_back(vertexp);
        }
    }

عمومی:
    GraphAlgStrongly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        اصلی();
    }
    ~GraphAlgStrongly() = پیشفرض;
};

تهی V3Graph::stronglyConnected(V3EdgeFuncP edgeFuncp) { GraphAlgStrongly{این, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking
// Changes user() and rank()

رده GraphAlgRank final : GraphAlg<> {
    تهی اصلی() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        برای (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        برای (V3GraphVertex& vertex : m_graphp->vertices()) {
            اگر (!vertex.user()) vertexIterate(&vertex, 1);
        }
    }
    تهی vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // If larger rank is found, assign it and loop back through
        // If we hit a back node make a list of all loops
        اگر (vertexp->user() == 1) {
            m_graphp->loopsMessageCb(vertexp, m_edgeFuncp);
            بازگشت;  // LCOV_EXCL_LINE  // gcc gprof bug misses this return
        }
        اگر (vertexp->rank() >= currentRank) بازگشت;  // Already processed it
        vertexp->user(1);
        vertexp->rank(currentRank);
        برای (V3GraphEdge& edge : vertexp->outEdges()) {
            اگر (followEdge(&edge)) vertexIterate(edge.بالاترین(), currentRank + vertexp->rankAdder());
        }
        vertexp->user(2);
    }

عمومی:
    GraphAlgRank(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        اصلی();
    }
    ~GraphAlgRank() = پیشفرض;
};

تهی V3Graph::rank() { rank(&V3GraphEdge::followAlwaysTrue); }
تهی V3Graph::rank(V3EdgeFuncP edgeFuncp) { GraphAlgRank{این, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking min
// Changes user() and rank()

رده GraphAlgRankMin final : GraphAlg<> {
    تهی اصلی() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        برای (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        برای (V3GraphVertex& vertex : m_graphp->vertices()) {
            اگر (!vertex.user()) vertexIterate(&vertex);
        }
    }
    uint32_t vertexIterate(V3GraphVertex* vertexp) {
        // Assign rank to each unvisited node
        // If we hit a back node make a list of all loops
        اگر (vertexp->user() == 1) {
            m_graphp->loopsMessageCb(vertexp, m_edgeFuncp);
            بازگشت vertexp->rank();
        }
        اگر (vertexp->user()) بازگشت vertexp->rank();  // Done earlier
        vertexp->user(1);
        vertexp->rank(1);  // In case loop
        // If no input edges, then rank 1 (+ adder)
        // Otherwise, get minimum from following all inputs.
        uint32_t minrank = ~0U;
        برای (V3GraphEdge& edge : vertexp->inEdges()) {
            اگر (followEdge(&edge)) {
                ثابت uint32_t nrank = vertexIterate(edge.fromp());
                minrank = std::min(minrank, nrank);
            }
        }
        اگر (minrank == ~0U) minrank = 0;
        vertexp->rank(minrank + vertexp->rankAdder());
        vertexp->user(2);
        بازگشت vertexp->rank();
    }

عمومی:
    GraphAlgRankMin(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        اصلی();
    }
    ~GraphAlgRankMin() = پیشفرض;
};

تهی V3Graph::rankMin() { rankMin(&V3GraphEdge::followAlwaysTrue); }
تهی V3Graph::rankMin(V3EdgeFuncP edgeFuncp) { GraphAlgRankMin{این, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - report loops
// Changes user()

رده GraphAlgRLoops final : GraphAlg<> {
    std::vector<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far
    std::vector<string> m_msgs;  // Output messages
    منطقی m_done = نادرست;  // Exit algorithm

    تهی اصلی(V3GraphVertex* vertexp) {
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        m_graphp->userClearVertices();
        m_callTrace.reserve(100);
        vertexIterate(vertexp, 0);
    }

    تهی vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // When we hit ourself again, return the list of all loops
        اگر (m_done) بازگشت;

        // Can't just reserve(), unless we modify size() before setting array directly
        وقتی (m_callTrace.size() <= currentRank) m_callTrace.افزودن_back(vertexp);
        m_callTrace[currentRank++] = vertexp;

        اگر (vertexp->user() == 1) {
            برای (unsigned i = 0; i < currentRank; i++)
                m_msgs.emplace_back(m_graphp->loopsVertexCb(m_callTrace[i]));
            m_done = درست;
            بازگشت;
        }
        اگر (vertexp->user() == 2) بازگشت;  // Already processed it
        vertexp->user(1);
        برای (V3GraphEdge& edge : vertexp->outEdges()) {
            اگر (followEdge(&edge)) vertexIterate(edge.بالاترین(), currentRank);
        }
        vertexp->user(2);
    }

عمومی:
    GraphAlgRLoops(V3Graph* graphp, V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp)
        : GraphAlg<>{graphp, edgeFuncp} {
        اصلی(vertexp);
    }
    ~GraphAlgRLoops() = پیشفرض;
    string message() ثابت {
        بازگشت std::accumulate(m_msgs.begin(), m_msgs.end(), std::string{""});
    }
};

string V3Graph::reportLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp) {
    بازگشت GraphAlgRLoops{این, edgeFuncp, vertexp}.message();
}

//######################################################################
//######################################################################
// Algorithms - subtrees
// Changes user()

رده GraphAlgSubtrees final : GraphAlg<> {
    V3Graph* ثابت m_loopGraphp;

    //! Iterate through all connected nodes of a graph with a loop or loops.
    V3GraphVertex* vertexIterateAll(V3GraphVertex* vertexp) {
        اگر (V3GraphVertex* newVertexp = static_cast<V3GraphVertex*>(vertexp->userp())) {
            بازگشت newVertexp;
        } وگرنه {
            newVertexp = vertexp->clone(m_loopGraphp);
            vertexp->userp(newVertexp);

            برای (V3GraphEdge& edge : vertexp->outEdges()) {
                اگر (followEdge(&edge)) {
                    V3GraphEdge* newEdgep = static_cast<V3GraphEdge*>(edge.userp());
                    اگر (!newEdgep) {
                        V3GraphVertex* new_بالاترین = vertexIterateAll(edge.بالاترین());
                        newEdgep = edge.clone(m_loopGraphp, newVertexp, new_بالاترین);
                        edge.userp(newEdgep);
                    }
                }
            }
            بازگشت newVertexp;
        }
    }

عمومی:
    GraphAlgSubtrees(V3Graph* graphp, V3Graph* loopGraphp, V3EdgeFuncP edgeFuncp,
                     V3GraphVertex* vertexp)
        : GraphAlg<>{graphp, edgeFuncp}
        , m_loopGraphp{loopGraphp} {
        // Vertex::m_userp - New vertex if we have seen this vertex already
        // Edge::m_userp - New edge if we have seen this edge already
        m_graphp->userClearVertices();
        m_graphp->userClearEdges();
        (تهی)vertexIterateAll(vertexp);
    }
    ~GraphAlgSubtrees() = پیشفرض;
};

//! Report the entire connected graph with a loop or loops
تهی V3Graph::subtreeLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp, V3Graph* loopGraphp) {
    GraphAlgSubtrees{این, loopGraphp, edgeFuncp, vertexp};
}

//######################################################################
//######################################################################
// Algorithms - sorting

تهی V3Graph::sortVertices() {
    // Sort list of vertices by rank, then fanout
    std::vector<V3GraphVertex*> vertexps;
    برای (V3GraphVertex& vertex : m_vertices) vertexps.افزودن_back(&vertex);
    std::stable_sort(vertexps.begin(), vertexps.end(),
                     [](ثابت V3GraphVertex* lhsp, ثابت V3GraphVertex* rhsp) {  //
                         بازگشت lhsp->sortCmp(rhsp) < 0;
                     });
    // Re-insert in sorted order
    برای (V3GraphVertex* ثابت vertexp : vertexps) {
        m_vertices.unlink(vertexp);
        m_vertices.linkBack(vertexp);
    }
}

تهی V3Graph::sortEdges() {
    // Sort edges by rank then fanout of node they point to
    std::vector<V3GraphEdge*> edges;
    برای (V3GraphVertex& vertex : vertices()) {
        // Make a vector
        برای (V3GraphEdge& edge : vertex.outEdges()) edges.افزودن_back(&edge);
        // Sort
        std::stable_sort(edges.begin(), edges.end(),
                         [](ثابت V3GraphEdge* lhsp, ثابت V3GraphEdge* rhsp) {  //
                             بازگشت lhsp->sortCmp(rhsp) < 0;
                         });
        // Relink edges in specified order
        برای (V3GraphEdge* ثابت edgep : edges) edgep->relinkFromp(&vertex);
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

تهی V3Graph::order() {
    UINFO(2, "Order:");

    // Compute rankings again
    rank(&V3GraphEdge::followAlwaysTrue);
    orderPreRanked();
}

تهی V3Graph::orderPreRanked() {
    // Compute fanouts
    // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
    userClearVertices();
    برای (V3GraphVertex& vertex : vertices()) {
        اگر (!vertex.user()) orderDFSIterate(&vertex);
    }

    // Sort list of vertices by rank, then fanout. Fanout is a bit of a
    // misnomer. It is the sum of all the fanouts of nodes reached from a node
    // *plus* the count of edges in to that node.
    sortVertices();
    // Sort edges by rank then fanout of node they point to
    sortEdges();
}

دوگانه V3Graph::orderDFSIterate(V3GraphVertex* vertexp) {
    // Compute fanouts of each node
    // If forward edge, don't double count that fanout
    اگر (vertexp->user() == 2) بازگشت vertexp->fanout();  // Already processed it
    UASSERT_OBJ(vertexp->user() != 1, vertexp, "Loop found, backward edges should be dead");
    vertexp->user(1);
    دوگانه fanout = 0;
    برای (V3GraphEdge& edge : vertexp->outEdges()) {
        اگر (edge.weight()) fanout += orderDFSIterate(edge.بالاترین());
    }
    // Just count inbound edges
    برای (V3GraphEdge& edge : vertexp->inEdges()) {
        اگر (edge.weight()) ++fanout;
    }
    vertexp->fanout(fanout);
    vertexp->user(2);
    بازگشت vertexp->fanout();
}

//######################################################################
//######################################################################
// Algorithms - parallelism report

رده GraphAlgParallelismReport final {
    // MEMBERS
    V3Graph& m_graph;  // The graph
    ثابت std::function<uint64_t(ثابت V3GraphVertex*)> m_vertexCost;  // vertex cost function
    V3Graph::ParallelismReport m_report;  // The result report

    // CONSTRUCTORS
    explicit GraphAlgParallelismReport(V3Graph& graph,
                                       std::function<uint64_t(ثابت V3GraphVertex*)> vertexCost)
        : m_graph{graph}
        , m_vertexCost{vertexCost} {
        // For each node, record the critical path cost from the start
        // of the graph through the end of the node.
        std::unordered_map<ثابت V3GraphVertex*, uint64_t> critPaths;
        GraphStreamUnordered serialize{&m_graph};
        برای (ثابت V3GraphVertex* vertexp; (vertexp = serialize.nextp());) {
            ++m_report.m_vertex_شمار;
            uint64_t cpCostToHere = 0;
            برای (ثابت V3GraphEdge& edge : vertexp->inEdges()) {
                ++m_report.m_edge_شمار;
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
    ~GraphAlgParallelismReport() = پیشفرض;
    VL_UNCOPYABLE(GraphAlgParallelismReport);
    VL_UNMOVABLE(GraphAlgParallelismReport);

عمومی:
    ایستا V3Graph::ParallelismReport
    apply(V3Graph& graph, std::function<uint32_t(ثابت V3GraphVertex*)> vertexCost) {
        بازگشت GraphAlgParallelismReport(graph, vertexCost).m_report;
    }
};

V3Graph::ParallelismReport
V3Graph::parallelismReport(std::function<uint64_t(ثابت V3GraphVertex*)> vertexCost) {
    بازگشت GraphAlgParallelismReport::apply(*این, vertexCost);
}
