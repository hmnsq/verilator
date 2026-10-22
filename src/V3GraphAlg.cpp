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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3GraphAlg.h"

#অন্তর্ভাব "V3Global.h"
#অন্তর্ভাব "V3GraphPathChecker.h"
#অন্তর্ভাব "V3GraphStream.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <limits>
#অন্তর্ভাব <list>
#অন্তর্ভাব <map>
#অন্তর্ভাব <numeric>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - Remove redundancies
// Changes user() and weight()

শ্রেণী GraphRemoveRedundant final : GraphAlg<> {
    ধ্রুবক যৌক্তিক m_যোগ_Weights;  // Sum, rather then maximize weights
ব্যক্তিগত:
    শূন্য প্রধান() {
        জন্য (V3GraphVertex& vertex : m_graphp->vertices()) vertexIterate(&vertex);
    }
    শূন্য vertexIterate(V3GraphVertex* vertexp) {
        // Clear marks
        জন্য (V3GraphEdge& edge : vertexp->outEdges()) edge.শীর্ষ()->userp(nullptr);

        // Mark edges and detect duplications

        জন্য (V3GraphEdge* ধ্রুবক edgep : vertexp->outEdges().unlinkable()) {
            যদি (followEdge(edgep)) {
                V3GraphVertex* outVertexp = edgep->শীর্ষ();
                V3GraphEdge* পূর্ব_Edgep = static_cast<V3GraphEdge*>(outVertexp->userp());
                যদি (!পূর্ব_Edgep) {  // No previous assignment
                    outVertexp->userp(edgep);
                } অন্যথা {  // Duplicate
                    যৌক্তিক saveOld = সত্য;
                    যদি (পূর্ব_Edgep->cutable() && !edgep->cutable()) {
                        saveOld = মিথ্যা;  // new !cutable more important than old
                    } অন্যথা যদি (!পূর্ব_Edgep->cutable() && edgep->cutable()) {
                        saveOld = সত্য;  // old !cutable more important than new
                    } অন্যথা {
                        saveOld = সত্য;
                        যদি (!m_যোগ_Weights
                            && (পূর্ব_Edgep->weight() < edgep->weight())) {  // Keep max weight
                            পূর্ব_Edgep->weight(edgep->weight());
                        }
                    }
                    যদি (saveOld) {
                        যদি (m_যোগ_Weights) পূর্ব_Edgep->weight(পূর্ব_Edgep->weight() + edgep->weight());
                        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    } অন্যথা {
                        যদি (m_যোগ_Weights) edgep->weight(পূর্ব_Edgep->weight() + edgep->weight());
                        VL_DO_DANGLING(পূর্ব_Edgep->unlinkDelete(), পূর্ব_Edgep);
                        outVertexp->userp(edgep);
                    }
                }
            }
        }
    }

সর্বজনীন:
    GraphRemoveRedundant(V3Graph* graphp, V3EdgeFuncP edgeFuncp, যৌক্তিক যোগ_Weights)
        : GraphAlg<>{graphp, edgeFuncp}
        , m_যোগ_Weights{যোগ_Weights} {
        প্রধান();
    }
    ~GraphRemoveRedundant() = পূর্বনির্ধারিত;
};

শূন্য V3Graph::removeRedundantEdgesMax(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{এই, edgeFuncp, মিথ্যা};
}
শূন্য V3Graph::remove_Redundant_Edges_যোগ(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{এই, edgeFuncp, সত্য};
}

//######################################################################
//######################################################################
// Algorithms - remove transitive

শ্রেণী GraphAlgRemoveTransitiveEdges final : GraphAlg<> {
সর্বজনীন:
    explicit GraphAlgRemoveTransitiveEdges(V3Graph* graphp)
        : GraphAlg<>(graphp, nullptr) {}
    শূন্য go() {
        GraphPathChecker checker{m_graphp};
        জন্য (V3GraphVertex& vtx : m_graphp->vertices()) {
            V3GraphEdge* deletep = nullptr;
            জন্য (V3GraphEdge& edge : vtx.outEdges()) {
                যদি (deletep) VL_DO_CLEAR(deletep->unlinkDelete(), deletep = nullptr);
                // It should be safe to modify the graph, despite using
                // the GraphPathChecker, as none of the modifications will
                // change what can be reached from what, nor should they
                // change the rank or CP of any node.
                যদি (checker.isTransitiveEdge(&edge)) deletep = &edge;
            }
            যদি (deletep) VL_DO_DANGLING(deletep->unlinkDelete(), deletep);
        }
    }

ব্যক্তিগত:
    VL_UNCOPYABLE(GraphAlgRemoveTransitiveEdges);
};

শূন্য V3Graph::removeTransitiveEdges() { GraphAlgRemoveTransitiveEdges{এই}.go(); }

//######################################################################
//######################################################################
// Algorithms - weakly connected components
// Changes color()

শ্রেণী GraphAlgWeakly final : GraphAlg<> {
    // Sentinel color, meaning not colored yet. Colors themselves are 0 .. m_numColors-1.
    স্থির constexpr uint32_t UNCOLORED = std::numeric_limits<uint32_t>::max();

    uint32_t m_অঙ্ক_Colors = 0;  // Number of colors assigned

    শূন্য প্রধান() {
        // Initialize state
        জন্য (V3GraphVertex& vertex : m_graphp->vertices()) vertex.color(UNCOLORED);
        // Color graph, without gaps
        জন্য (V3GraphVertex& vertex : m_graphp->vertices()) {
            যদি (vertex.color() == UNCOLORED) vertexIterate(&vertex, m_অঙ্ক_Colors++);
        }
    }

    শূন্য vertexIterate(V3GraphVertex* vertexp, uint32_t currentColor) {
        // Assign new color to each unvisited node
        // then visit each of its edges, giving them the same color
        যদি (vertexp->color() != UNCOLORED) ফেরত;  // Already colored it
        vertexp->color(currentColor);
        জন্য (V3GraphEdge& edge : vertexp->outEdges()) {
            যদি (followEdge(&edge)) vertexIterate(edge.শীর্ষ(), currentColor);
        }
        জন্য (V3GraphEdge& edge : vertexp->inEdges()) {
            যদি (followEdge(&edge)) vertexIterate(edge.fromp(), currentColor);
        }
    }

সর্বজনীন:
    GraphAlgWeakly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>(graphp, edgeFuncp) {
        প্রধান();
    }
    ~GraphAlgWeakly() = পূর্বনির্ধারিত;

    uint32_t অঙ্ক_Colors() ধ্রুবক { ফেরত m_অঙ্ক_Colors; }
};

uint32_t V3Graph::weaklyConnected(V3EdgeFuncP edgeFuncp) {
    ফেরত GraphAlgWeakly{এই, edgeFuncp}.অঙ্ক_Colors();
}

//######################################################################
//######################################################################
// Algorithms - strongly connected components
// Changes user() and color()

শ্রেণী GraphAlgStrongly final : GraphAlg<> {
    uint32_t m_currentDfs = 0;  // DFS count
    std::vector<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far

    শূন্য প্রধান() {
        // Use Pearce's algorithm to color the strongly connected components. For reference see
        // "An Improved Algorithm for Finding the Strongly Connected Components of a Directed
        // Graph", David J.Pearce, 2005
        //
        // Node State:
        //     Vertex::user     // DFS number indicating possible root of subtree, 0=not iterated
        //     Vertex::color    // Output subtree number (fully processed)

        // Clear info
        জন্য (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.color(0);
            vertex.user(0);
        }
        // Color graph
        জন্য (V3GraphVertex& vertex : m_graphp->vertices()) {
            যদি (!vertex.user()) {
                m_currentDfs++;
                vertexIterate(&vertex);
            }
        }
        // If there's a single vertex of a color, it doesn't need a subgraph
        // This simplifies the consumer's code, and reduces graph debugging clutter
        জন্য (V3GraphVertex& vertex : m_graphp->vertices()) {
            যৌক্তিক onecolor = সত্য;
            জন্য (V3GraphEdge& edge : vertex.outEdges()) {
                যদি (followEdge(&edge)) {
                    যদি (vertex.color() == edge.শীর্ষ()->color()) {
                        onecolor = মিথ্যা;
                        বিরতি;
                    }
                }
            }
            যদি (onecolor) vertex.color(0);
        }
    }

    শূন্য vertexIterate(V3GraphVertex* vertexp) {
        ধ্রুবক uint32_t this_Dfs_অঙ্ক = m_currentDfs++;
        vertexp->user(this_Dfs_অঙ্ক);
        vertexp->color(0);
        জন্য (V3GraphEdge& edge : vertexp->outEdges()) {
            যদি (followEdge(&edge)) {
                V3GraphVertex* শীর্ষ = edge.শীর্ষ();
                যদি (!শীর্ষ->user()) {  // Dest not computed yet
                    vertexIterate(শীর্ষ);
                }
                যদি (!শীর্ষ->color()) {  // Dest not in a component
                    যদি (vertexp->user() > শীর্ষ->user()) vertexp->user(শীর্ষ->user());
                }
            }
        }
        যদি (vertexp->user() == this_Dfs_অঙ্ক) {  // New head of subtree
            vertexp->color(this_Dfs_অঙ্ক);  // Mark as component
            যখন (!m_callTrace.empty()) {
                V3GraphVertex* বের_করো_Vertexp = m_callTrace.back();
                যদি (বের_করো_Vertexp->user() >= this_Dfs_অঙ্ক) {  // Lower node is part of this subtree
                    m_callTrace.বের_করো_back();
                    বের_করো_Vertexp->color(this_Dfs_অঙ্ক);
                } অন্যথা {
                    বিরতি;
                }
            }
        } অন্যথা {  // In another subtree (maybe...)
            m_callTrace.ঠেলো_back(vertexp);
        }
    }

সর্বজনীন:
    GraphAlgStrongly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        প্রধান();
    }
    ~GraphAlgStrongly() = পূর্বনির্ধারিত;
};

শূন্য V3Graph::stronglyConnected(V3EdgeFuncP edgeFuncp) { GraphAlgStrongly{এই, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking
// Changes user() and rank()

শ্রেণী GraphAlgRank final : GraphAlg<> {
    শূন্য প্রধান() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        জন্য (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        জন্য (V3GraphVertex& vertex : m_graphp->vertices()) {
            যদি (!vertex.user()) vertexIterate(&vertex, 1);
        }
    }
    শূন্য vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // If larger rank is found, assign it and loop back through
        // If we hit a back node make a list of all loops
        যদি (vertexp->user() == 1) {
            m_graphp->loopsMessageCb(vertexp, m_edgeFuncp);
            ফেরত;  // LCOV_EXCL_LINE  // gcc gprof bug misses this return
        }
        যদি (vertexp->rank() >= currentRank) ফেরত;  // Already processed it
        vertexp->user(1);
        vertexp->rank(currentRank);
        জন্য (V3GraphEdge& edge : vertexp->outEdges()) {
            যদি (followEdge(&edge)) vertexIterate(edge.শীর্ষ(), currentRank + vertexp->rankAdder());
        }
        vertexp->user(2);
    }

সর্বজনীন:
    GraphAlgRank(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        প্রধান();
    }
    ~GraphAlgRank() = পূর্বনির্ধারিত;
};

শূন্য V3Graph::rank() { rank(&V3GraphEdge::followAlwaysTrue); }
শূন্য V3Graph::rank(V3EdgeFuncP edgeFuncp) { GraphAlgRank{এই, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking min
// Changes user() and rank()

শ্রেণী GraphAlgRankMin final : GraphAlg<> {
    শূন্য প্রধান() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        জন্য (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        জন্য (V3GraphVertex& vertex : m_graphp->vertices()) {
            যদি (!vertex.user()) vertexIterate(&vertex);
        }
    }
    uint32_t vertexIterate(V3GraphVertex* vertexp) {
        // Assign rank to each unvisited node
        // If we hit a back node make a list of all loops
        যদি (vertexp->user() == 1) {
            m_graphp->loopsMessageCb(vertexp, m_edgeFuncp);
            ফেরত vertexp->rank();
        }
        যদি (vertexp->user()) ফেরত vertexp->rank();  // Done earlier
        vertexp->user(1);
        vertexp->rank(1);  // In case loop
        // If no input edges, then rank 1 (+ adder)
        // Otherwise, get minimum from following all inputs.
        uint32_t minrank = ~0U;
        জন্য (V3GraphEdge& edge : vertexp->inEdges()) {
            যদি (followEdge(&edge)) {
                ধ্রুবক uint32_t nrank = vertexIterate(edge.fromp());
                minrank = std::min(minrank, nrank);
            }
        }
        যদি (minrank == ~0U) minrank = 0;
        vertexp->rank(minrank + vertexp->rankAdder());
        vertexp->user(2);
        ফেরত vertexp->rank();
    }

সর্বজনীন:
    GraphAlgRankMin(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        প্রধান();
    }
    ~GraphAlgRankMin() = পূর্বনির্ধারিত;
};

শূন্য V3Graph::rankMin() { rankMin(&V3GraphEdge::followAlwaysTrue); }
শূন্য V3Graph::rankMin(V3EdgeFuncP edgeFuncp) { GraphAlgRankMin{এই, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - report loops
// Changes user()

শ্রেণী GraphAlgRLoops final : GraphAlg<> {
    std::vector<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far
    std::vector<string> m_msgs;  // Output messages
    যৌক্তিক m_done = মিথ্যা;  // Exit algorithm

    শূন্য প্রধান(V3GraphVertex* vertexp) {
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        m_graphp->userClearVertices();
        m_callTrace.reserve(100);
        vertexIterate(vertexp, 0);
    }

    শূন্য vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // When we hit ourself again, return the list of all loops
        যদি (m_done) ফেরত;

        // Can't just reserve(), unless we modify size() before setting array directly
        যখন (m_callTrace.আকার_2() <= currentRank) m_callTrace.ঠেলো_back(vertexp);
        m_callTrace[currentRank++] = vertexp;

        যদি (vertexp->user() == 1) {
            জন্য (unsigned i = 0; i < currentRank; i++)
                m_msgs.emplace_back(m_graphp->loopsVertexCb(m_callTrace[i]));
            m_done = সত্য;
            ফেরত;
        }
        যদি (vertexp->user() == 2) ফেরত;  // Already processed it
        vertexp->user(1);
        জন্য (V3GraphEdge& edge : vertexp->outEdges()) {
            যদি (followEdge(&edge)) vertexIterate(edge.শীর্ষ(), currentRank);
        }
        vertexp->user(2);
    }

সর্বজনীন:
    GraphAlgRLoops(V3Graph* graphp, V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp)
        : GraphAlg<>{graphp, edgeFuncp} {
        প্রধান(vertexp);
    }
    ~GraphAlgRLoops() = পূর্বনির্ধারিত;
    string message() ধ্রুবক {
        ফেরত std::accumulate(m_msgs.begin(), m_msgs.end(), std::string{""});
    }
};

string V3Graph::reportLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp) {
    ফেরত GraphAlgRLoops{এই, edgeFuncp, vertexp}.message();
}

//######################################################################
//######################################################################
// Algorithms - subtrees
// Changes user()

শ্রেণী GraphAlgSubtrees final : GraphAlg<> {
    V3Graph* ধ্রুবক m_loopGraphp;

    //! Iterate through all connected nodes of a graph with a loop or loops.
    V3GraphVertex* vertexIterateAll(V3GraphVertex* vertexp) {
        যদি (V3GraphVertex* newVertexp = static_cast<V3GraphVertex*>(vertexp->userp())) {
            ফেরত newVertexp;
        } অন্যথা {
            newVertexp = vertexp->clone(m_loopGraphp);
            vertexp->userp(newVertexp);

            জন্য (V3GraphEdge& edge : vertexp->outEdges()) {
                যদি (followEdge(&edge)) {
                    V3GraphEdge* newEdgep = static_cast<V3GraphEdge*>(edge.userp());
                    যদি (!newEdgep) {
                        V3GraphVertex* new_শীর্ষ = vertexIterateAll(edge.শীর্ষ());
                        newEdgep = edge.clone(m_loopGraphp, newVertexp, new_শীর্ষ);
                        edge.userp(newEdgep);
                    }
                }
            }
            ফেরত newVertexp;
        }
    }

সর্বজনীন:
    GraphAlgSubtrees(V3Graph* graphp, V3Graph* loopGraphp, V3EdgeFuncP edgeFuncp,
                     V3GraphVertex* vertexp)
        : GraphAlg<>{graphp, edgeFuncp}
        , m_loopGraphp{loopGraphp} {
        // Vertex::m_userp - New vertex if we have seen this vertex already
        // Edge::m_userp - New edge if we have seen this edge already
        m_graphp->userClearVertices();
        m_graphp->userClearEdges();
        (শূন্য)vertexIterateAll(vertexp);
    }
    ~GraphAlgSubtrees() = পূর্বনির্ধারিত;
};

//! Report the entire connected graph with a loop or loops
শূন্য V3Graph::subtreeLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp, V3Graph* loopGraphp) {
    GraphAlgSubtrees{এই, loopGraphp, edgeFuncp, vertexp};
}

//######################################################################
//######################################################################
// Algorithms - sorting

শূন্য V3Graph::sortVertices() {
    // Sort list of vertices by rank, then fanout
    std::vector<V3GraphVertex*> vertexps;
    জন্য (V3GraphVertex& vertex : m_vertices) vertexps.ঠেলো_back(&vertex);
    std::stable_sort(vertexps.begin(), vertexps.end(),
                     [](ধ্রুবক V3GraphVertex* lhsp, ধ্রুবক V3GraphVertex* rhsp) {  //
                         ফেরত lhsp->sortCmp(rhsp) < 0;
                     });
    // Re-insert in sorted order
    জন্য (V3GraphVertex* ধ্রুবক vertexp : vertexps) {
        m_vertices.unlink(vertexp);
        m_vertices.linkBack(vertexp);
    }
}

শূন্য V3Graph::sortEdges() {
    // Sort edges by rank then fanout of node they point to
    std::vector<V3GraphEdge*> edges;
    জন্য (V3GraphVertex& vertex : vertices()) {
        // Make a vector
        জন্য (V3GraphEdge& edge : vertex.outEdges()) edges.ঠেলো_back(&edge);
        // Sort
        std::stable_sort(edges.begin(), edges.end(),
                         [](ধ্রুবক V3GraphEdge* lhsp, ধ্রুবক V3GraphEdge* rhsp) {  //
                             ফেরত lhsp->sortCmp(rhsp) < 0;
                         });
        // Relink edges in specified order
        জন্য (V3GraphEdge* ধ্রুবক edgep : edges) edgep->relinkFromp(&vertex);
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

শূন্য V3Graph::order() {
    UINFO(2, "Order:");

    // Compute rankings again
    rank(&V3GraphEdge::followAlwaysTrue);
    orderPreRanked();
}

শূন্য V3Graph::orderPreRanked() {
    // Compute fanouts
    // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
    userClearVertices();
    জন্য (V3GraphVertex& vertex : vertices()) {
        যদি (!vertex.user()) orderDFSIterate(&vertex);
    }

    // Sort list of vertices by rank, then fanout. Fanout is a bit of a
    // misnomer. It is the sum of all the fanouts of nodes reached from a node
    // *plus* the count of edges in to that node.
    sortVertices();
    // Sort edges by rank then fanout of node they point to
    sortEdges();
}

দ্বিগুণ V3Graph::orderDFSIterate(V3GraphVertex* vertexp) {
    // Compute fanouts of each node
    // If forward edge, don't double count that fanout
    যদি (vertexp->user() == 2) ফেরত vertexp->fanout();  // Already processed it
    UASSERT_OBJ(vertexp->user() != 1, vertexp, "Loop found, backward edges should be dead");
    vertexp->user(1);
    দ্বিগুণ fanout = 0;
    জন্য (V3GraphEdge& edge : vertexp->outEdges()) {
        যদি (edge.weight()) fanout += orderDFSIterate(edge.শীর্ষ());
    }
    // Just count inbound edges
    জন্য (V3GraphEdge& edge : vertexp->inEdges()) {
        যদি (edge.weight()) ++fanout;
    }
    vertexp->fanout(fanout);
    vertexp->user(2);
    ফেরত vertexp->fanout();
}

//######################################################################
//######################################################################
// Algorithms - parallelism report

শ্রেণী GraphAlgParallelismReport final {
    // MEMBERS
    V3Graph& m_graph;  // The graph
    ধ্রুবক std::function<uint64_t(ধ্রুবক V3GraphVertex*)> m_vertexCost;  // vertex cost function
    V3Graph::ParallelismReport m_report;  // The result report

    // CONSTRUCTORS
    explicit GraphAlgParallelismReport(V3Graph& graph,
                                       std::function<uint64_t(ধ্রুবক V3GraphVertex*)> vertexCost)
        : m_graph{graph}
        , m_vertexCost{vertexCost} {
        // For each node, record the critical path cost from the start
        // of the graph through the end of the node.
        std::unordered_map<ধ্রুবক V3GraphVertex*, uint64_t> critPaths;
        GraphStreamUnordered serialize{&m_graph};
        জন্য (ধ্রুবক V3GraphVertex* vertexp; (vertexp = serialize.nextp());) {
            ++m_report.m_vertex_গুনতি;
            uint64_t cpCostToHere = 0;
            জন্য (ধ্রুবক V3GraphEdge& edge : vertexp->inEdges()) {
                ++m_report.m_edge_গুনতি;
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
            m_report.m_যোগ_Graph_Cost += m_vertexCost(vertexp);
        }
    }
    ~GraphAlgParallelismReport() = পূর্বনির্ধারিত;
    VL_UNCOPYABLE(GraphAlgParallelismReport);
    VL_UNMOVABLE(GraphAlgParallelismReport);

সর্বজনীন:
    স্থির V3Graph::ParallelismReport
    apply(V3Graph& graph, std::function<uint32_t(ধ্রুবক V3GraphVertex*)> vertexCost) {
        ফেরত GraphAlgParallelismReport(graph, vertexCost).m_report;
    }
};

V3Graph::ParallelismReport
V3Graph::parallelismReport(std::function<uint64_t(ধ্রুবক V3GraphVertex*)> vertexCost) {
    ফেরত GraphAlgParallelismReport::apply(*এই, vertexCost);
}
