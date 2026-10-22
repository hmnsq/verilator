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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3GraphAlg.h"

#ضمن "V3Global.h"
#ضمن "V3GraphPathChecker.h"
#ضمن "V3GraphStream.h"
#ضمن "V3Stats.h"

#ضمن <algorithm>
#ضمن <limits>
#ضمن <list>
#ضمن <map>
#ضمن <numeric>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - Remove redundancies
// Changes user() and weight()

صنف GraphRemoveRedundant final : GraphAlg<> {
    ثابت منطقي m_sumWeights;  // Sum, rather then maximize weights
خاص:
    فراغ main() {
        لكل (V3GraphVertex& vertex : m_graphp->vertices()) vertexIterate(&vertex);
    }
    فراغ vertexIterate(V3GraphVertex* vertexp) {
        // Clear marks
        لكل (V3GraphEdge& edge : vertexp->outEdges()) edge.top()->userp(nullptr);

        // Mark edges and detect duplications

        لكل (V3GraphEdge* ثابت edgep : vertexp->outEdges().unlinkable()) {
            𐤀𐤌 (followEdge(edgep)) {
                V3GraphVertex* outVertexp = edgep->top();
                V3GraphEdge* prevEdgep = static_cast<V3GraphEdge*>(outVertexp->userp());
                𐤀𐤌 (!prevEdgep) {  // No previous assignment
                    outVertexp->userp(edgep);
                } وإلا {  // Duplicate
                    منطقي saveOld = true;
                    𐤀𐤌 (prevEdgep->cutable() && !edgep->cutable()) {
                        saveOld = خطأ;  // new !cutable more important than old
                    } وإلا 𐤀𐤌 (!prevEdgep->cutable() && edgep->cutable()) {
                        saveOld = true;  // old !cutable more important than new
                    } وإلا {
                        saveOld = true;
                        𐤀𐤌 (!m_sumWeights
                            && (prevEdgep->weight() < edgep->weight())) {  // Keep max weight
                            prevEdgep->weight(edgep->weight());
                        }
                    }
                    𐤀𐤌 (saveOld) {
                        𐤀𐤌 (m_sumWeights) prevEdgep->weight(prevEdgep->weight() + edgep->weight());
                        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    } وإلا {
                        𐤀𐤌 (m_sumWeights) edgep->weight(prevEdgep->weight() + edgep->weight());
                        VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                        outVertexp->userp(edgep);
                    }
                }
            }
        }
    }

عام:
    GraphRemoveRedundant(V3Graph* graphp, V3EdgeFuncP edgeFuncp, منطقي sumWeights)
        : GraphAlg<>{graphp, edgeFuncp}
        , m_sumWeights{sumWeights} {
        main();
    }
    ~GraphRemoveRedundant() = افتراضي;
};

فراغ V3Graph::removeRedundantEdgesMax(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{هذا, edgeFuncp, خطأ};
}
فراغ V3Graph::removeRedundantEdgesSum(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{هذا, edgeFuncp, true};
}

//######################################################################
//######################################################################
// Algorithms - remove transitive

صنف GraphAlgRemoveTransitiveEdges final : GraphAlg<> {
عام:
    explicit GraphAlgRemoveTransitiveEdges(V3Graph* graphp)
        : GraphAlg<>(graphp, nullptr) {}
    فراغ go() {
        GraphPathChecker checker{m_graphp};
        لكل (V3GraphVertex& vtx : m_graphp->vertices()) {
            V3GraphEdge* deletep = nullptr;
            لكل (V3GraphEdge& edge : vtx.outEdges()) {
                𐤀𐤌 (deletep) VL_DO_CLEAR(deletep->unlinkDelete(), deletep = nullptr);
                // It should be safe to modify the graph, despite using
                // the GraphPathChecker, as none of the modifications will
                // change what can be reached from what, nor should they
                // change the rank or CP of any node.
                𐤀𐤌 (checker.isTransitiveEdge(&edge)) deletep = &edge;
            }
            𐤀𐤌 (deletep) VL_DO_DANGLING(deletep->unlinkDelete(), deletep);
        }
    }

خاص:
    VL_UNCOPYABLE(GraphAlgRemoveTransitiveEdges);
};

فراغ V3Graph::removeTransitiveEdges() { GraphAlgRemoveTransitiveEdges{هذا}.go(); }

//######################################################################
//######################################################################
// Algorithms - weakly connected components
// Changes color()

صنف GraphAlgWeakly final : GraphAlg<> {
    // Sentinel color, meaning not colored yet. Colors themselves are 0 .. m_numColors-1.
    ساكن constexpr uint32_t UNCOLORED = std::numeric_limits<uint32_t>::max();

    uint32_t m_numColors = 0;  // Number of colors assigned

    فراغ main() {
        // Initialize state
        لكل (V3GraphVertex& vertex : m_graphp->vertices()) vertex.color(UNCOLORED);
        // Color graph, without gaps
        لكل (V3GraphVertex& vertex : m_graphp->vertices()) {
            𐤀𐤌 (vertex.color() == UNCOLORED) vertexIterate(&vertex, m_numColors++);
        }
    }

    فراغ vertexIterate(V3GraphVertex* vertexp, uint32_t currentColor) {
        // Assign new color to each unvisited node
        // then visit each of its edges, giving them the same color
        𐤀𐤌 (vertexp->color() != UNCOLORED) أرجع;  // Already colored it
        vertexp->color(currentColor);
        لكل (V3GraphEdge& edge : vertexp->outEdges()) {
            𐤀𐤌 (followEdge(&edge)) vertexIterate(edge.top(), currentColor);
        }
        لكل (V3GraphEdge& edge : vertexp->inEdges()) {
            𐤀𐤌 (followEdge(&edge)) vertexIterate(edge.fromp(), currentColor);
        }
    }

عام:
    GraphAlgWeakly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>(graphp, edgeFuncp) {
        main();
    }
    ~GraphAlgWeakly() = افتراضي;

    uint32_t numColors() ثابت { أرجع m_numColors; }
};

uint32_t V3Graph::weaklyConnected(V3EdgeFuncP edgeFuncp) {
    أرجع GraphAlgWeakly{هذا, edgeFuncp}.numColors();
}

//######################################################################
//######################################################################
// Algorithms - strongly connected components
// Changes user() and color()

صنف GraphAlgStrongly final : GraphAlg<> {
    uint32_t m_currentDfs = 0;  // DFS count
    std::vector<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far

    فراغ main() {
        // Use Pearce's algorithm to color the strongly connected components. For reference see
        // "An Improved Algorithm for Finding the Strongly Connected Components of a Directed
        // Graph", David J.Pearce, 2005
        //
        // Node State:
        //     Vertex::user     // DFS number indicating possible root of subtree, 0=not iterated
        //     Vertex::color    // Output subtree number (fully processed)

        // Clear info
        لكل (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.color(0);
            vertex.user(0);
        }
        // Color graph
        لكل (V3GraphVertex& vertex : m_graphp->vertices()) {
            𐤀𐤌 (!vertex.user()) {
                m_currentDfs++;
                vertexIterate(&vertex);
            }
        }
        // If there's a single vertex of a color, it doesn't need a subgraph
        // This simplifies the consumer's code, and reduces graph debugging clutter
        لكل (V3GraphVertex& vertex : m_graphp->vertices()) {
            منطقي onecolor = true;
            لكل (V3GraphEdge& edge : vertex.outEdges()) {
                𐤀𐤌 (followEdge(&edge)) {
                    𐤀𐤌 (vertex.color() == edge.top()->color()) {
                        onecolor = خطأ;
                        اكسر;
                    }
                }
            }
            𐤀𐤌 (onecolor) vertex.color(0);
        }
    }

    فراغ vertexIterate(V3GraphVertex* vertexp) {
        ثابت uint32_t thisDfsNum = m_currentDfs++;
        vertexp->user(thisDfsNum);
        vertexp->color(0);
        لكل (V3GraphEdge& edge : vertexp->outEdges()) {
            𐤀𐤌 (followEdge(&edge)) {
                V3GraphVertex* top = edge.top();
                𐤀𐤌 (!top->user()) {  // Dest not computed yet
                    vertexIterate(top);
                }
                𐤀𐤌 (!top->color()) {  // Dest not in a component
                    𐤀𐤌 (vertexp->user() > top->user()) vertexp->user(top->user());
                }
            }
        }
        𐤀𐤌 (vertexp->user() == thisDfsNum) {  // New head of subtree
            vertexp->color(thisDfsNum);  // Mark as component
            طالما (!m_callTrace.empty()) {
                V3GraphVertex* popVertexp = m_callTrace.back();
                𐤀𐤌 (popVertexp->user() >= thisDfsNum) {  // Lower node is part of this subtree
                    m_callTrace.pop_back();
                    popVertexp->color(thisDfsNum);
                } وإلا {
                    اكسر;
                }
            }
        } وإلا {  // In another subtree (maybe...)
            m_callTrace.push_back(vertexp);
        }
    }

عام:
    GraphAlgStrongly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        main();
    }
    ~GraphAlgStrongly() = افتراضي;
};

فراغ V3Graph::stronglyConnected(V3EdgeFuncP edgeFuncp) { GraphAlgStrongly{هذا, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking
// Changes user() and rank()

صنف GraphAlgRank final : GraphAlg<> {
    فراغ main() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        لكل (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        لكل (V3GraphVertex& vertex : m_graphp->vertices()) {
            𐤀𐤌 (!vertex.user()) vertexIterate(&vertex, 1);
        }
    }
    فراغ vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // If larger rank is found, assign it and loop back through
        // If we hit a back node make a list of all loops
        𐤀𐤌 (vertexp->user() == 1) {
            m_graphp->loopsMessageCb(vertexp, m_edgeFuncp);
            أرجع;  // LCOV_EXCL_LINE  // gcc gprof bug misses this return
        }
        𐤀𐤌 (vertexp->rank() >= currentRank) أرجع;  // Already processed it
        vertexp->user(1);
        vertexp->rank(currentRank);
        لكل (V3GraphEdge& edge : vertexp->outEdges()) {
            𐤀𐤌 (followEdge(&edge)) vertexIterate(edge.top(), currentRank + vertexp->rankAdder());
        }
        vertexp->user(2);
    }

عام:
    GraphAlgRank(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        main();
    }
    ~GraphAlgRank() = افتراضي;
};

فراغ V3Graph::rank() { rank(&V3GraphEdge::followAlwaysTrue); }
فراغ V3Graph::rank(V3EdgeFuncP edgeFuncp) { GraphAlgRank{هذا, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking min
// Changes user() and rank()

صنف GraphAlgRankMin final : GraphAlg<> {
    فراغ main() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        لكل (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        لكل (V3GraphVertex& vertex : m_graphp->vertices()) {
            𐤀𐤌 (!vertex.user()) vertexIterate(&vertex);
        }
    }
    uint32_t vertexIterate(V3GraphVertex* vertexp) {
        // Assign rank to each unvisited node
        // If we hit a back node make a list of all loops
        𐤀𐤌 (vertexp->user() == 1) {
            m_graphp->loopsMessageCb(vertexp, m_edgeFuncp);
            أرجع vertexp->rank();
        }
        𐤀𐤌 (vertexp->user()) أرجع vertexp->rank();  // Done earlier
        vertexp->user(1);
        vertexp->rank(1);  // In case loop
        // If no input edges, then rank 1 (+ adder)
        // Otherwise, get minimum from following all inputs.
        uint32_t minrank = ~0U;
        لكل (V3GraphEdge& edge : vertexp->inEdges()) {
            𐤀𐤌 (followEdge(&edge)) {
                ثابت uint32_t nrank = vertexIterate(edge.fromp());
                minrank = std::min(minrank, nrank);
            }
        }
        𐤀𐤌 (minrank == ~0U) minrank = 0;
        vertexp->rank(minrank + vertexp->rankAdder());
        vertexp->user(2);
        أرجع vertexp->rank();
    }

عام:
    GraphAlgRankMin(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        main();
    }
    ~GraphAlgRankMin() = افتراضي;
};

فراغ V3Graph::rankMin() { rankMin(&V3GraphEdge::followAlwaysTrue); }
فراغ V3Graph::rankMin(V3EdgeFuncP edgeFuncp) { GraphAlgRankMin{هذا, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - report loops
// Changes user()

صنف GraphAlgRLoops final : GraphAlg<> {
    std::vector<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far
    std::vector<string> m_msgs;  // Output messages
    منطقي m_done = خطأ;  // Exit algorithm

    فراغ main(V3GraphVertex* vertexp) {
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        m_graphp->userClearVertices();
        m_callTrace.reserve(100);
        vertexIterate(vertexp, 0);
    }

    فراغ vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // When we hit ourself again, return the list of all loops
        𐤀𐤌 (m_done) أرجع;

        // Can't just reserve(), unless we modify size() before setting array directly
        طالما (m_callTrace.size() <= currentRank) m_callTrace.push_back(vertexp);
        m_callTrace[currentRank++] = vertexp;

        𐤀𐤌 (vertexp->user() == 1) {
            لكل (unsigned i = 0; i < currentRank; i++)
                m_msgs.emplace_back(m_graphp->loopsVertexCb(m_callTrace[i]));
            m_done = true;
            أرجع;
        }
        𐤀𐤌 (vertexp->user() == 2) أرجع;  // Already processed it
        vertexp->user(1);
        لكل (V3GraphEdge& edge : vertexp->outEdges()) {
            𐤀𐤌 (followEdge(&edge)) vertexIterate(edge.top(), currentRank);
        }
        vertexp->user(2);
    }

عام:
    GraphAlgRLoops(V3Graph* graphp, V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp)
        : GraphAlg<>{graphp, edgeFuncp} {
        main(vertexp);
    }
    ~GraphAlgRLoops() = افتراضي;
    string message() ثابت {
        أرجع std::accumulate(m_msgs.begin(), m_msgs.end(), std::string{""});
    }
};

string V3Graph::reportLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp) {
    أرجع GraphAlgRLoops{هذا, edgeFuncp, vertexp}.message();
}

//######################################################################
//######################################################################
// Algorithms - subtrees
// Changes user()

صنف GraphAlgSubtrees final : GraphAlg<> {
    V3Graph* ثابت m_loopGraphp;

    //! Iterate through all connected nodes of a graph with a loop or loops.
    V3GraphVertex* vertexIterateAll(V3GraphVertex* vertexp) {
        𐤀𐤌 (V3GraphVertex* newVertexp = static_cast<V3GraphVertex*>(vertexp->userp())) {
            أرجع newVertexp;
        } وإلا {
            newVertexp = vertexp->clone(m_loopGraphp);
            vertexp->userp(newVertexp);

            لكل (V3GraphEdge& edge : vertexp->outEdges()) {
                𐤀𐤌 (followEdge(&edge)) {
                    V3GraphEdge* newEdgep = static_cast<V3GraphEdge*>(edge.userp());
                    𐤀𐤌 (!newEdgep) {
                        V3GraphVertex* newTop = vertexIterateAll(edge.top());
                        newEdgep = edge.clone(m_loopGraphp, newVertexp, newTop);
                        edge.userp(newEdgep);
                    }
                }
            }
            أرجع newVertexp;
        }
    }

عام:
    GraphAlgSubtrees(V3Graph* graphp, V3Graph* loopGraphp, V3EdgeFuncP edgeFuncp,
                     V3GraphVertex* vertexp)
        : GraphAlg<>{graphp, edgeFuncp}
        , m_loopGraphp{loopGraphp} {
        // Vertex::m_userp - New vertex if we have seen this vertex already
        // Edge::m_userp - New edge if we have seen this edge already
        m_graphp->userClearVertices();
        m_graphp->userClearEdges();
        (فراغ)vertexIterateAll(vertexp);
    }
    ~GraphAlgSubtrees() = افتراضي;
};

//! Report the entire connected graph with a loop or loops
فراغ V3Graph::subtreeLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp, V3Graph* loopGraphp) {
    GraphAlgSubtrees{هذا, loopGraphp, edgeFuncp, vertexp};
}

//######################################################################
//######################################################################
// Algorithms - sorting

فراغ V3Graph::sortVertices() {
    // Sort list of vertices by rank, then fanout
    std::vector<V3GraphVertex*> vertexps;
    لكل (V3GraphVertex& vertex : m_vertices) vertexps.push_back(&vertex);
    std::stable_sort(vertexps.begin(), vertexps.end(),
                     [](ثابت V3GraphVertex* lhsp, ثابت V3GraphVertex* rhsp) {  //
                         أرجع lhsp->sortCmp(rhsp) < 0;
                     });
    // Re-insert in sorted order
    لكل (V3GraphVertex* ثابت vertexp : vertexps) {
        m_vertices.unlink(vertexp);
        m_vertices.linkBack(vertexp);
    }
}

فراغ V3Graph::sortEdges() {
    // Sort edges by rank then fanout of node they point to
    std::vector<V3GraphEdge*> edges;
    لكل (V3GraphVertex& vertex : vertices()) {
        // Make a vector
        لكل (V3GraphEdge& edge : vertex.outEdges()) edges.push_back(&edge);
        // Sort
        std::stable_sort(edges.begin(), edges.end(),
                         [](ثابت V3GraphEdge* lhsp, ثابت V3GraphEdge* rhsp) {  //
                             أرجع lhsp->sortCmp(rhsp) < 0;
                         });
        // Relink edges in specified order
        لكل (V3GraphEdge* ثابت edgep : edges) edgep->relinkFromp(&vertex);
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

فراغ V3Graph::order() {
    UINFO(2, "Order:");

    // Compute rankings again
    rank(&V3GraphEdge::followAlwaysTrue);
    orderPreRanked();
}

فراغ V3Graph::orderPreRanked() {
    // Compute fanouts
    // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
    userClearVertices();
    لكل (V3GraphVertex& vertex : vertices()) {
        𐤀𐤌 (!vertex.user()) orderDFSIterate(&vertex);
    }

    // Sort list of vertices by rank, then fanout. Fanout is a bit of a
    // misnomer. It is the sum of all the fanouts of nodes reached from a node
    // *plus* the count of edges in to that node.
    sortVertices();
    // Sort edges by rank then fanout of node they point to
    sortEdges();
}

مزدوج V3Graph::orderDFSIterate(V3GraphVertex* vertexp) {
    // Compute fanouts of each node
    // If forward edge, don't double count that fanout
    𐤀𐤌 (vertexp->user() == 2) أرجع vertexp->fanout();  // Already processed it
    UASSERT_OBJ(vertexp->user() != 1, vertexp, "Loop found, backward edges should be dead");
    vertexp->user(1);
    مزدوج fanout = 0;
    لكل (V3GraphEdge& edge : vertexp->outEdges()) {
        𐤀𐤌 (edge.weight()) fanout += orderDFSIterate(edge.top());
    }
    // Just count inbound edges
    لكل (V3GraphEdge& edge : vertexp->inEdges()) {
        𐤀𐤌 (edge.weight()) ++fanout;
    }
    vertexp->fanout(fanout);
    vertexp->user(2);
    أرجع vertexp->fanout();
}

//######################################################################
//######################################################################
// Algorithms - parallelism report

صنف GraphAlgParallelismReport final {
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
        لكل (ثابت V3GraphVertex* vertexp; (vertexp = serialize.nextp());) {
            ++m_report.m_vertexCount;
            uint64_t cpCostToHere = 0;
            لكل (ثابت V3GraphEdge& edge : vertexp->inEdges()) {
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
    ~GraphAlgParallelismReport() = افتراضي;
    VL_UNCOPYABLE(GraphAlgParallelismReport);
    VL_UNMOVABLE(GraphAlgParallelismReport);

عام:
    ساكن V3Graph::ParallelismReport
    apply(V3Graph& graph, std::function<uint32_t(ثابت V3GraphVertex*)> vertexCost) {
        أرجع GraphAlgParallelismReport(graph, vertexCost).m_report;
    }
};

V3Graph::ParallelismReport
V3Graph::parallelismReport(std::function<uint64_t(ثابت V3GraphVertex*)> vertexCost) {
    أرجع GraphAlgParallelismReport::apply(*هذا, vertexCost);
}
