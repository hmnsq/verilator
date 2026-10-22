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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3GraphAlg.h"

#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3GraphPathChecker.h"
#அந்தர்பாவ "V3GraphStream.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <limits>
#அந்தர்பாவ <list>
#அந்தர்பாவ <map>
#அந்தர்பாவ <numeric>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - Remove redundancies
// Changes user() and weight()

வகுப்பு GraphRemoveRedundant final : GraphAlg<> {
    மாறிலி தருக்க m_sumWeights;  // Sum, rather then maximize weights
தனிப்பட்ட:
    வெறுமை முதன்மை() {
        க்கு (V3GraphVertex& vertex : m_graphp->vertices()) vertexIterate(&vertex);
    }
    வெறுமை vertexIterate(V3GraphVertex* vertexp) {
        // Clear marks
        க்கு (V3GraphEdge& edge : vertexp->outEdges()) edge.உச்சி()->userp(nullptr);

        // Mark edges and detect duplications

        க்கு (V3GraphEdge* மாறிலி edgep : vertexp->outEdges().unlinkable()) {
            எனில் (followEdge(edgep)) {
                V3GraphVertex* outVertexp = edgep->உச்சி();
                V3GraphEdge* prevEdgep = static_cast<V3GraphEdge*>(outVertexp->userp());
                எனில் (!prevEdgep) {  // No previous assignment
                    outVertexp->userp(edgep);
                } இல்லையெனில் {  // Duplicate
                    தருக்க saveOld = உண்மை;
                    எனில் (prevEdgep->cutable() && !edgep->cutable()) {
                        saveOld = பொய்;  // new !cutable more important than old
                    } இல்லையெனில் எனில் (!prevEdgep->cutable() && edgep->cutable()) {
                        saveOld = உண்மை;  // old !cutable more important than new
                    } இல்லையெனில் {
                        saveOld = உண்மை;
                        எனில் (!m_sumWeights
                            && (prevEdgep->weight() < edgep->weight())) {  // Keep max weight
                            prevEdgep->weight(edgep->weight());
                        }
                    }
                    எனில் (saveOld) {
                        எனில் (m_sumWeights) prevEdgep->weight(prevEdgep->weight() + edgep->weight());
                        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    } இல்லையெனில் {
                        எனில் (m_sumWeights) edgep->weight(prevEdgep->weight() + edgep->weight());
                        VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                        outVertexp->userp(edgep);
                    }
                }
            }
        }
    }

பொது:
    GraphRemoveRedundant(V3Graph* graphp, V3EdgeFuncP edgeFuncp, தருக்க sumWeights)
        : GraphAlg<>{graphp, edgeFuncp}
        , m_sumWeights{sumWeights} {
        முதன்மை();
    }
    ~GraphRemoveRedundant() = இயல்பு;
};

வெறுமை V3Graph::removeRedundantEdgesMax(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{இது, edgeFuncp, பொய்};
}
வெறுமை V3Graph::removeRedundantEdgesSum(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{இது, edgeFuncp, உண்மை};
}

//######################################################################
//######################################################################
// Algorithms - remove transitive

வகுப்பு GraphAlgRemoveTransitiveEdges final : GraphAlg<> {
பொது:
    explicit GraphAlgRemoveTransitiveEdges(V3Graph* graphp)
        : GraphAlg<>(graphp, nullptr) {}
    வெறுமை go() {
        GraphPathChecker checker{m_graphp};
        க்கு (V3GraphVertex& vtx : m_graphp->vertices()) {
            V3GraphEdge* deletep = nullptr;
            க்கு (V3GraphEdge& edge : vtx.outEdges()) {
                எனில் (deletep) VL_DO_CLEAR(deletep->unlinkDelete(), deletep = nullptr);
                // It should be safe to modify the graph, despite using
                // the GraphPathChecker, as none of the modifications will
                // change what can be reached from what, nor should they
                // change the rank or CP of any node.
                எனில் (checker.isTransitiveEdge(&edge)) deletep = &edge;
            }
            எனில் (deletep) VL_DO_DANGLING(deletep->unlinkDelete(), deletep);
        }
    }

தனிப்பட்ட:
    VL_UNCOPYABLE(GraphAlgRemoveTransitiveEdges);
};

வெறுமை V3Graph::removeTransitiveEdges() { GraphAlgRemoveTransitiveEdges{இது}.go(); }

//######################################################################
//######################################################################
// Algorithms - weakly connected components
// Changes color()

வகுப்பு GraphAlgWeakly final : GraphAlg<> {
    // Sentinel color, meaning not colored yet. Colors themselves are 0 .. m_numColors-1.
    நிலையான constexpr uint32_t UNCOLORED = std::numeric_limits<uint32_t>::max();

    uint32_t m_இலக்கம்_Colors = 0;  // Number of colors assigned

    வெறுமை முதன்மை() {
        // Initialize state
        க்கு (V3GraphVertex& vertex : m_graphp->vertices()) vertex.color(UNCOLORED);
        // Color graph, without gaps
        க்கு (V3GraphVertex& vertex : m_graphp->vertices()) {
            எனில் (vertex.color() == UNCOLORED) vertexIterate(&vertex, m_இலக்கம்_Colors++);
        }
    }

    வெறுமை vertexIterate(V3GraphVertex* vertexp, uint32_t currentColor) {
        // Assign new color to each unvisited node
        // then visit each of its edges, giving them the same color
        எனில் (vertexp->color() != UNCOLORED) திரும்பு;  // Already colored it
        vertexp->color(currentColor);
        க்கு (V3GraphEdge& edge : vertexp->outEdges()) {
            எனில் (followEdge(&edge)) vertexIterate(edge.உச்சி(), currentColor);
        }
        க்கு (V3GraphEdge& edge : vertexp->inEdges()) {
            எனில் (followEdge(&edge)) vertexIterate(edge.fromp(), currentColor);
        }
    }

பொது:
    GraphAlgWeakly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>(graphp, edgeFuncp) {
        முதன்மை();
    }
    ~GraphAlgWeakly() = இயல்பு;

    uint32_t இலக்கம்_Colors() மாறிலி { திரும்பு m_இலக்கம்_Colors; }
};

uint32_t V3Graph::weaklyConnected(V3EdgeFuncP edgeFuncp) {
    திரும்பு GraphAlgWeakly{இது, edgeFuncp}.இலக்கம்_Colors();
}

//######################################################################
//######################################################################
// Algorithms - strongly connected components
// Changes user() and color()

வகுப்பு GraphAlgStrongly final : GraphAlg<> {
    uint32_t m_currentDfs = 0;  // DFS count
    std::vector<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far

    வெறுமை முதன்மை() {
        // Use Pearce's algorithm to color the strongly connected components. For reference see
        // "An Improved Algorithm for Finding the Strongly Connected Components of a Directed
        // Graph", David J.Pearce, 2005
        //
        // Node State:
        //     Vertex::user     // DFS number indicating possible root of subtree, 0=not iterated
        //     Vertex::color    // Output subtree number (fully processed)

        // Clear info
        க்கு (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.color(0);
            vertex.user(0);
        }
        // Color graph
        க்கு (V3GraphVertex& vertex : m_graphp->vertices()) {
            எனில் (!vertex.user()) {
                m_currentDfs++;
                vertexIterate(&vertex);
            }
        }
        // If there's a single vertex of a color, it doesn't need a subgraph
        // This simplifies the consumer's code, and reduces graph debugging clutter
        க்கு (V3GraphVertex& vertex : m_graphp->vertices()) {
            தருக்க onecolor = உண்மை;
            க்கு (V3GraphEdge& edge : vertex.outEdges()) {
                எனில் (followEdge(&edge)) {
                    எனில் (vertex.color() == edge.உச்சி()->color()) {
                        onecolor = பொய்;
                        நிறுத்து;
                    }
                }
            }
            எனில் (onecolor) vertex.color(0);
        }
    }

    வெறுமை vertexIterate(V3GraphVertex* vertexp) {
        மாறிலி uint32_t this_Dfs_இலக்கம் = m_currentDfs++;
        vertexp->user(this_Dfs_இலக்கம்);
        vertexp->color(0);
        க்கு (V3GraphEdge& edge : vertexp->outEdges()) {
            எனில் (followEdge(&edge)) {
                V3GraphVertex* உச்சி = edge.உச்சி();
                எனில் (!உச்சி->user()) {  // Dest not computed yet
                    vertexIterate(உச்சி);
                }
                எனில் (!உச்சி->color()) {  // Dest not in a component
                    எனில் (vertexp->user() > உச்சி->user()) vertexp->user(உச்சி->user());
                }
            }
        }
        எனில் (vertexp->user() == this_Dfs_இலக்கம்) {  // New head of subtree
            vertexp->color(this_Dfs_இலக்கம்);  // Mark as component
            போது (!m_callTrace.empty()) {
                V3GraphVertex* எடு_Vertexp = m_callTrace.back();
                எனில் (எடு_Vertexp->user() >= this_Dfs_இலக்கம்) {  // Lower node is part of this subtree
                    m_callTrace.எடு_back();
                    எடு_Vertexp->color(this_Dfs_இலக்கம்);
                } இல்லையெனில் {
                    நிறுத்து;
                }
            }
        } இல்லையெனில் {  // In another subtree (maybe...)
            m_callTrace.தள்ளு_back(vertexp);
        }
    }

பொது:
    GraphAlgStrongly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        முதன்மை();
    }
    ~GraphAlgStrongly() = இயல்பு;
};

வெறுமை V3Graph::stronglyConnected(V3EdgeFuncP edgeFuncp) { GraphAlgStrongly{இது, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking
// Changes user() and rank()

வகுப்பு GraphAlgRank final : GraphAlg<> {
    வெறுமை முதன்மை() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        க்கு (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        க்கு (V3GraphVertex& vertex : m_graphp->vertices()) {
            எனில் (!vertex.user()) vertexIterate(&vertex, 1);
        }
    }
    வெறுமை vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // If larger rank is found, assign it and loop back through
        // If we hit a back node make a list of all loops
        எனில் (vertexp->user() == 1) {
            m_graphp->loopsMessageCb(vertexp, m_edgeFuncp);
            திரும்பு;  // LCOV_EXCL_LINE  // gcc gprof bug misses this return
        }
        எனில் (vertexp->rank() >= currentRank) திரும்பு;  // Already processed it
        vertexp->user(1);
        vertexp->rank(currentRank);
        க்கு (V3GraphEdge& edge : vertexp->outEdges()) {
            எனில் (followEdge(&edge)) vertexIterate(edge.உச்சி(), currentRank + vertexp->rankAdder());
        }
        vertexp->user(2);
    }

பொது:
    GraphAlgRank(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        முதன்மை();
    }
    ~GraphAlgRank() = இயல்பு;
};

வெறுமை V3Graph::rank() { rank(&V3GraphEdge::followAlwaysTrue); }
வெறுமை V3Graph::rank(V3EdgeFuncP edgeFuncp) { GraphAlgRank{இது, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking min
// Changes user() and rank()

வகுப்பு GraphAlgRankMin final : GraphAlg<> {
    வெறுமை முதன்மை() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        க்கு (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        க்கு (V3GraphVertex& vertex : m_graphp->vertices()) {
            எனில் (!vertex.user()) vertexIterate(&vertex);
        }
    }
    uint32_t vertexIterate(V3GraphVertex* vertexp) {
        // Assign rank to each unvisited node
        // If we hit a back node make a list of all loops
        எனில் (vertexp->user() == 1) {
            m_graphp->loopsMessageCb(vertexp, m_edgeFuncp);
            திரும்பு vertexp->rank();
        }
        எனில் (vertexp->user()) திரும்பு vertexp->rank();  // Done earlier
        vertexp->user(1);
        vertexp->rank(1);  // In case loop
        // If no input edges, then rank 1 (+ adder)
        // Otherwise, get minimum from following all inputs.
        uint32_t minrank = ~0U;
        க்கு (V3GraphEdge& edge : vertexp->inEdges()) {
            எனில் (followEdge(&edge)) {
                மாறிலி uint32_t nrank = vertexIterate(edge.fromp());
                minrank = std::min(minrank, nrank);
            }
        }
        எனில் (minrank == ~0U) minrank = 0;
        vertexp->rank(minrank + vertexp->rankAdder());
        vertexp->user(2);
        திரும்பு vertexp->rank();
    }

பொது:
    GraphAlgRankMin(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        முதன்மை();
    }
    ~GraphAlgRankMin() = இயல்பு;
};

வெறுமை V3Graph::rankMin() { rankMin(&V3GraphEdge::followAlwaysTrue); }
வெறுமை V3Graph::rankMin(V3EdgeFuncP edgeFuncp) { GraphAlgRankMin{இது, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - report loops
// Changes user()

வகுப்பு GraphAlgRLoops final : GraphAlg<> {
    std::vector<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far
    std::vector<string> m_msgs;  // Output messages
    தருக்க m_done = பொய்;  // Exit algorithm

    வெறுமை முதன்மை(V3GraphVertex* vertexp) {
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        m_graphp->userClearVertices();
        m_callTrace.reserve(100);
        vertexIterate(vertexp, 0);
    }

    வெறுமை vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // When we hit ourself again, return the list of all loops
        எனில் (m_done) திரும்பு;

        // Can't just reserve(), unless we modify size() before setting array directly
        போது (m_callTrace.size() <= currentRank) m_callTrace.தள்ளு_back(vertexp);
        m_callTrace[currentRank++] = vertexp;

        எனில் (vertexp->user() == 1) {
            க்கு (unsigned i = 0; i < currentRank; i++)
                m_msgs.emplace_back(m_graphp->loopsVertexCb(m_callTrace[i]));
            m_done = உண்மை;
            திரும்பு;
        }
        எனில் (vertexp->user() == 2) திரும்பு;  // Already processed it
        vertexp->user(1);
        க்கு (V3GraphEdge& edge : vertexp->outEdges()) {
            எனில் (followEdge(&edge)) vertexIterate(edge.உச்சி(), currentRank);
        }
        vertexp->user(2);
    }

பொது:
    GraphAlgRLoops(V3Graph* graphp, V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp)
        : GraphAlg<>{graphp, edgeFuncp} {
        முதன்மை(vertexp);
    }
    ~GraphAlgRLoops() = இயல்பு;
    string message() மாறிலி {
        திரும்பு std::accumulate(m_msgs.begin(), m_msgs.end(), std::string{""});
    }
};

string V3Graph::reportLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp) {
    திரும்பு GraphAlgRLoops{இது, edgeFuncp, vertexp}.message();
}

//######################################################################
//######################################################################
// Algorithms - subtrees
// Changes user()

வகுப்பு GraphAlgSubtrees final : GraphAlg<> {
    V3Graph* மாறிலி m_loopGraphp;

    //! Iterate through all connected nodes of a graph with a loop or loops.
    V3GraphVertex* vertexIterateAll(V3GraphVertex* vertexp) {
        எனில் (V3GraphVertex* newVertexp = static_cast<V3GraphVertex*>(vertexp->userp())) {
            திரும்பு newVertexp;
        } இல்லையெனில் {
            newVertexp = vertexp->clone(m_loopGraphp);
            vertexp->userp(newVertexp);

            க்கு (V3GraphEdge& edge : vertexp->outEdges()) {
                எனில் (followEdge(&edge)) {
                    V3GraphEdge* newEdgep = static_cast<V3GraphEdge*>(edge.userp());
                    எனில் (!newEdgep) {
                        V3GraphVertex* new_உச்சி = vertexIterateAll(edge.உச்சி());
                        newEdgep = edge.clone(m_loopGraphp, newVertexp, new_உச்சி);
                        edge.userp(newEdgep);
                    }
                }
            }
            திரும்பு newVertexp;
        }
    }

பொது:
    GraphAlgSubtrees(V3Graph* graphp, V3Graph* loopGraphp, V3EdgeFuncP edgeFuncp,
                     V3GraphVertex* vertexp)
        : GraphAlg<>{graphp, edgeFuncp}
        , m_loopGraphp{loopGraphp} {
        // Vertex::m_userp - New vertex if we have seen this vertex already
        // Edge::m_userp - New edge if we have seen this edge already
        m_graphp->userClearVertices();
        m_graphp->userClearEdges();
        (வெறுமை)vertexIterateAll(vertexp);
    }
    ~GraphAlgSubtrees() = இயல்பு;
};

//! Report the entire connected graph with a loop or loops
வெறுமை V3Graph::subtreeLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp, V3Graph* loopGraphp) {
    GraphAlgSubtrees{இது, loopGraphp, edgeFuncp, vertexp};
}

//######################################################################
//######################################################################
// Algorithms - sorting

வெறுமை V3Graph::sortVertices() {
    // Sort list of vertices by rank, then fanout
    std::vector<V3GraphVertex*> vertexps;
    க்கு (V3GraphVertex& vertex : m_vertices) vertexps.தள்ளு_back(&vertex);
    std::stable_sort(vertexps.begin(), vertexps.end(),
                     [](மாறிலி V3GraphVertex* lhsp, மாறிலி V3GraphVertex* rhsp) {  //
                         திரும்பு lhsp->sortCmp(rhsp) < 0;
                     });
    // Re-insert in sorted order
    க்கு (V3GraphVertex* மாறிலி vertexp : vertexps) {
        m_vertices.unlink(vertexp);
        m_vertices.linkBack(vertexp);
    }
}

வெறுமை V3Graph::sortEdges() {
    // Sort edges by rank then fanout of node they point to
    std::vector<V3GraphEdge*> edges;
    க்கு (V3GraphVertex& vertex : vertices()) {
        // Make a vector
        க்கு (V3GraphEdge& edge : vertex.outEdges()) edges.தள்ளு_back(&edge);
        // Sort
        std::stable_sort(edges.begin(), edges.end(),
                         [](மாறிலி V3GraphEdge* lhsp, மாறிலி V3GraphEdge* rhsp) {  //
                             திரும்பு lhsp->sortCmp(rhsp) < 0;
                         });
        // Relink edges in specified order
        க்கு (V3GraphEdge* மாறிலி edgep : edges) edgep->relinkFromp(&vertex);
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

வெறுமை V3Graph::order() {
    UINFO(2, "Order:");

    // Compute rankings again
    rank(&V3GraphEdge::followAlwaysTrue);
    orderPreRanked();
}

வெறுமை V3Graph::orderPreRanked() {
    // Compute fanouts
    // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
    userClearVertices();
    க்கு (V3GraphVertex& vertex : vertices()) {
        எனில் (!vertex.user()) orderDFSIterate(&vertex);
    }

    // Sort list of vertices by rank, then fanout. Fanout is a bit of a
    // misnomer. It is the sum of all the fanouts of nodes reached from a node
    // *plus* the count of edges in to that node.
    sortVertices();
    // Sort edges by rank then fanout of node they point to
    sortEdges();
}

இரட்டை V3Graph::orderDFSIterate(V3GraphVertex* vertexp) {
    // Compute fanouts of each node
    // If forward edge, don't double count that fanout
    எனில் (vertexp->user() == 2) திரும்பு vertexp->fanout();  // Already processed it
    UASSERT_OBJ(vertexp->user() != 1, vertexp, "Loop found, backward edges should be dead");
    vertexp->user(1);
    இரட்டை fanout = 0;
    க்கு (V3GraphEdge& edge : vertexp->outEdges()) {
        எனில் (edge.weight()) fanout += orderDFSIterate(edge.உச்சி());
    }
    // Just count inbound edges
    க்கு (V3GraphEdge& edge : vertexp->inEdges()) {
        எனில் (edge.weight()) ++fanout;
    }
    vertexp->fanout(fanout);
    vertexp->user(2);
    திரும்பு vertexp->fanout();
}

//######################################################################
//######################################################################
// Algorithms - parallelism report

வகுப்பு GraphAlgParallelismReport final {
    // MEMBERS
    V3Graph& m_graph;  // The graph
    மாறிலி std::function<uint64_t(மாறிலி V3GraphVertex*)> m_vertexCost;  // vertex cost function
    V3Graph::ParallelismReport m_report;  // The result report

    // CONSTRUCTORS
    explicit GraphAlgParallelismReport(V3Graph& graph,
                                       std::function<uint64_t(மாறிலி V3GraphVertex*)> vertexCost)
        : m_graph{graph}
        , m_vertexCost{vertexCost} {
        // For each node, record the critical path cost from the start
        // of the graph through the end of the node.
        std::unordered_map<மாறிலி V3GraphVertex*, uint64_t> critPaths;
        GraphStreamUnordered serialize{&m_graph};
        க்கு (மாறிலி V3GraphVertex* vertexp; (vertexp = serialize.nextp());) {
            ++m_report.m_vertex_மொத்தம்;
            uint64_t cpCostToHere = 0;
            க்கு (மாறிலி V3GraphEdge& edge : vertexp->inEdges()) {
                ++m_report.m_edge_மொத்தம்;
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
    ~GraphAlgParallelismReport() = இயல்பு;
    VL_UNCOPYABLE(GraphAlgParallelismReport);
    VL_UNMOVABLE(GraphAlgParallelismReport);

பொது:
    நிலையான V3Graph::ParallelismReport
    apply(V3Graph& graph, std::function<uint32_t(மாறிலி V3GraphVertex*)> vertexCost) {
        திரும்பு GraphAlgParallelismReport(graph, vertexCost).m_report;
    }
};

V3Graph::ParallelismReport
V3Graph::parallelismReport(std::function<uint64_t(மாறிலி V3GraphVertex*)> vertexCost) {
    திரும்பு GraphAlgParallelismReport::apply(*இது, vertexCost);
}
