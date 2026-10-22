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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3GraphAlg.h"

#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3GraphPathChecker.h"
#ಅನ್ತರ್ಭಾವ "V3GraphStream.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <limits>
#ಅನ್ತರ್ಭಾವ <list>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <numeric>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - Remove redundancies
// Changes user() and weight()

ವರ್ಗ GraphRemoveRedundant final : GraphAlg<> {
    ಧ್ರುವ ತಾರ್ಕಿಕ m_sumWeights;  // Sum, rather then maximize weights
ಗುಪ್ತ:
    ಶೂನ್ಯ main() {
        ಪ್ರತಿ (V3GraphVertex& vertex : m_graphp->vertices()) vertexIterate(&vertex);
    }
    ಶೂನ್ಯ vertexIterate(V3GraphVertex* vertexp) {
        // Clear marks
        ಪ್ರತಿ (V3GraphEdge& edge : vertexp->outEdges()) edge.ತುದಿ()->userp(nullptr);

        // Mark edges and detect duplications

        ಪ್ರತಿ (V3GraphEdge* ಧ್ರುವ edgep : vertexp->outEdges().unlinkable()) {
            ಯದಿ (followEdge(edgep)) {
                V3GraphVertex* outVertexp = edgep->ತುದಿ();
                V3GraphEdge* prevEdgep = static_cast<V3GraphEdge*>(outVertexp->userp());
                ಯದಿ (!prevEdgep) {  // No previous assignment
                    outVertexp->userp(edgep);
                } ಅನ್ಯಥಾ {  // Duplicate
                    ತಾರ್ಕಿಕ saveOld = ಸತ್ಯ;
                    ಯದಿ (prevEdgep->cutable() && !edgep->cutable()) {
                        saveOld = ಅಸತ್ಯ;  // new !cutable more important than old
                    } ಅನ್ಯಥಾ ಯದಿ (!prevEdgep->cutable() && edgep->cutable()) {
                        saveOld = ಸತ್ಯ;  // old !cutable more important than new
                    } ಅನ್ಯಥಾ {
                        saveOld = ಸತ್ಯ;
                        ಯದಿ (!m_sumWeights
                            && (prevEdgep->weight() < edgep->weight())) {  // Keep max weight
                            prevEdgep->weight(edgep->weight());
                        }
                    }
                    ಯದಿ (saveOld) {
                        ಯದಿ (m_sumWeights) prevEdgep->weight(prevEdgep->weight() + edgep->weight());
                        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    } ಅನ್ಯಥಾ {
                        ಯದಿ (m_sumWeights) edgep->weight(prevEdgep->weight() + edgep->weight());
                        VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                        outVertexp->userp(edgep);
                    }
                }
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    GraphRemoveRedundant(V3Graph* graphp, V3EdgeFuncP edgeFuncp, ತಾರ್ಕಿಕ sumWeights)
        : GraphAlg<>{graphp, edgeFuncp}
        , m_sumWeights{sumWeights} {
        main();
    }
    ~GraphRemoveRedundant() = ಸ್ವಭಾವ;
};

ಶೂನ್ಯ V3Graph::removeRedundantEdgesMax(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{ಏತತ್, edgeFuncp, ಅಸತ್ಯ};
}
ಶೂನ್ಯ V3Graph::removeRedundantEdgesSum(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{ಏತತ್, edgeFuncp, ಸತ್ಯ};
}

//######################################################################
//######################################################################
// Algorithms - remove transitive

ವರ್ಗ GraphAlgRemoveTransitiveEdges final : GraphAlg<> {
ಸಾರ್ವಜನಿಕ:
    explicit GraphAlgRemoveTransitiveEdges(V3Graph* graphp)
        : GraphAlg<>(graphp, nullptr) {}
    ಶೂನ್ಯ go() {
        GraphPathChecker checker{m_graphp};
        ಪ್ರತಿ (V3GraphVertex& vtx : m_graphp->vertices()) {
            V3GraphEdge* deletep = nullptr;
            ಪ್ರತಿ (V3GraphEdge& edge : vtx.outEdges()) {
                ಯದಿ (deletep) VL_DO_CLEAR(deletep->unlinkDelete(), deletep = nullptr);
                // It should be safe to modify the graph, despite using
                // the GraphPathChecker, as none of the modifications will
                // change what can be reached from what, nor should they
                // change the rank or CP of any node.
                ಯದಿ (checker.isTransitiveEdge(&edge)) deletep = &edge;
            }
            ಯದಿ (deletep) VL_DO_DANGLING(deletep->unlinkDelete(), deletep);
        }
    }

ಗುಪ್ತ:
    VL_UNCOPYABLE(GraphAlgRemoveTransitiveEdges);
};

ಶೂನ್ಯ V3Graph::removeTransitiveEdges() { GraphAlgRemoveTransitiveEdges{ಏತತ್}.go(); }

//######################################################################
//######################################################################
// Algorithms - weakly connected components
// Changes color()

ವರ್ಗ GraphAlgWeakly final : GraphAlg<> {
    // Sentinel color, meaning not colored yet. Colors themselves are 0 .. m_numColors-1.
    ಸ್ಥಿರ constexpr uint32_t UNCOLORED = std::numeric_limits<uint32_t>::max();

    uint32_t m_ಅಂಕೆ_Colors = 0;  // Number of colors assigned

    ಶೂನ್ಯ main() {
        // Initialize state
        ಪ್ರತಿ (V3GraphVertex& vertex : m_graphp->vertices()) vertex.color(UNCOLORED);
        // Color graph, without gaps
        ಪ್ರತಿ (V3GraphVertex& vertex : m_graphp->vertices()) {
            ಯದಿ (vertex.color() == UNCOLORED) vertexIterate(&vertex, m_ಅಂಕೆ_Colors++);
        }
    }

    ಶೂನ್ಯ vertexIterate(V3GraphVertex* vertexp, uint32_t currentColor) {
        // Assign new color to each unvisited node
        // then visit each of its edges, giving them the same color
        ಯದಿ (vertexp->color() != UNCOLORED) ಪ್ರತ್ಯಾಗಮ;  // Already colored it
        vertexp->color(currentColor);
        ಪ್ರತಿ (V3GraphEdge& edge : vertexp->outEdges()) {
            ಯದಿ (followEdge(&edge)) vertexIterate(edge.ತುದಿ(), currentColor);
        }
        ಪ್ರತಿ (V3GraphEdge& edge : vertexp->inEdges()) {
            ಯದಿ (followEdge(&edge)) vertexIterate(edge.fromp(), currentColor);
        }
    }

ಸಾರ್ವಜನಿಕ:
    GraphAlgWeakly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>(graphp, edgeFuncp) {
        main();
    }
    ~GraphAlgWeakly() = ಸ್ವಭಾವ;

    uint32_t ಅಂಕೆ_Colors() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಅಂಕೆ_Colors; }
};

uint32_t V3Graph::weaklyConnected(V3EdgeFuncP edgeFuncp) {
    ಪ್ರತ್ಯಾಗಮ GraphAlgWeakly{ಏತತ್, edgeFuncp}.ಅಂಕೆ_Colors();
}

//######################################################################
//######################################################################
// Algorithms - strongly connected components
// Changes user() and color()

ವರ್ಗ GraphAlgStrongly final : GraphAlg<> {
    uint32_t m_currentDfs = 0;  // DFS count
    std::vector<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far

    ಶೂನ್ಯ main() {
        // Use Pearce's algorithm to color the strongly connected components. For reference see
        // "An Improved Algorithm for Finding the Strongly Connected Components of a Directed
        // Graph", David J.Pearce, 2005
        //
        // Node State:
        //     Vertex::user     // DFS number indicating possible root of subtree, 0=not iterated
        //     Vertex::color    // Output subtree number (fully processed)

        // Clear info
        ಪ್ರತಿ (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.color(0);
            vertex.user(0);
        }
        // Color graph
        ಪ್ರತಿ (V3GraphVertex& vertex : m_graphp->vertices()) {
            ಯದಿ (!vertex.user()) {
                m_currentDfs++;
                vertexIterate(&vertex);
            }
        }
        // If there's a single vertex of a color, it doesn't need a subgraph
        // This simplifies the consumer's code, and reduces graph debugging clutter
        ಪ್ರತಿ (V3GraphVertex& vertex : m_graphp->vertices()) {
            ತಾರ್ಕಿಕ onecolor = ಸತ್ಯ;
            ಪ್ರತಿ (V3GraphEdge& edge : vertex.outEdges()) {
                ಯದಿ (followEdge(&edge)) {
                    ಯದಿ (vertex.color() == edge.ತುದಿ()->color()) {
                        onecolor = ಅಸತ್ಯ;
                        ಭಙ್ಗ;
                    }
                }
            }
            ಯದಿ (onecolor) vertex.color(0);
        }
    }

    ಶೂನ್ಯ vertexIterate(V3GraphVertex* vertexp) {
        ಧ್ರುವ uint32_t this_Dfs_ಅಂಕೆ = m_currentDfs++;
        vertexp->user(this_Dfs_ಅಂಕೆ);
        vertexp->color(0);
        ಪ್ರತಿ (V3GraphEdge& edge : vertexp->outEdges()) {
            ಯದಿ (followEdge(&edge)) {
                V3GraphVertex* ತುದಿ = edge.ತುದಿ();
                ಯದಿ (!ತುದಿ->user()) {  // Dest not computed yet
                    vertexIterate(ತುದಿ);
                }
                ಯದಿ (!ತುದಿ->color()) {  // Dest not in a component
                    ಯದಿ (vertexp->user() > ತುದಿ->user()) vertexp->user(ತುದಿ->user());
                }
            }
        }
        ಯದಿ (vertexp->user() == this_Dfs_ಅಂಕೆ) {  // New head of subtree
            vertexp->color(this_Dfs_ಅಂಕೆ);  // Mark as component
            ಯಾವತ್ (!m_callTrace.empty()) {
                V3GraphVertex* ತೆಗೆ_Vertexp = m_callTrace.back();
                ಯದಿ (ತೆಗೆ_Vertexp->user() >= this_Dfs_ಅಂಕೆ) {  // Lower node is part of this subtree
                    m_callTrace.ತೆಗೆ_back();
                    ತೆಗೆ_Vertexp->color(this_Dfs_ಅಂಕೆ);
                } ಅನ್ಯಥಾ {
                    ಭಙ್ಗ;
                }
            }
        } ಅನ್ಯಥಾ {  // In another subtree (maybe...)
            m_callTrace.ತಳ್ಳು_back(vertexp);
        }
    }

ಸಾರ್ವಜನಿಕ:
    GraphAlgStrongly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        main();
    }
    ~GraphAlgStrongly() = ಸ್ವಭಾವ;
};

ಶೂನ್ಯ V3Graph::stronglyConnected(V3EdgeFuncP edgeFuncp) { GraphAlgStrongly{ಏತತ್, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking
// Changes user() and rank()

ವರ್ಗ GraphAlgRank final : GraphAlg<> {
    ಶೂನ್ಯ main() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        ಪ್ರತಿ (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        ಪ್ರತಿ (V3GraphVertex& vertex : m_graphp->vertices()) {
            ಯದಿ (!vertex.user()) vertexIterate(&vertex, 1);
        }
    }
    ಶೂನ್ಯ vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // If larger rank is found, assign it and loop back through
        // If we hit a back node make a list of all loops
        ಯದಿ (vertexp->user() == 1) {
            m_graphp->loopsMessageCb(vertexp, m_edgeFuncp);
            ಪ್ರತ್ಯಾಗಮ;  // LCOV_EXCL_LINE  // gcc gprof bug misses this return
        }
        ಯದಿ (vertexp->rank() >= currentRank) ಪ್ರತ್ಯಾಗಮ;  // Already processed it
        vertexp->user(1);
        vertexp->rank(currentRank);
        ಪ್ರತಿ (V3GraphEdge& edge : vertexp->outEdges()) {
            ಯದಿ (followEdge(&edge)) vertexIterate(edge.ತುದಿ(), currentRank + vertexp->rankAdder());
        }
        vertexp->user(2);
    }

ಸಾರ್ವಜನಿಕ:
    GraphAlgRank(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        main();
    }
    ~GraphAlgRank() = ಸ್ವಭಾವ;
};

ಶೂನ್ಯ V3Graph::rank() { rank(&V3GraphEdge::followAlwaysTrue); }
ಶೂನ್ಯ V3Graph::rank(V3EdgeFuncP edgeFuncp) { GraphAlgRank{ಏತತ್, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking min
// Changes user() and rank()

ವರ್ಗ GraphAlgRankMin final : GraphAlg<> {
    ಶೂನ್ಯ main() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        ಪ್ರತಿ (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        ಪ್ರತಿ (V3GraphVertex& vertex : m_graphp->vertices()) {
            ಯದಿ (!vertex.user()) vertexIterate(&vertex);
        }
    }
    uint32_t vertexIterate(V3GraphVertex* vertexp) {
        // Assign rank to each unvisited node
        // If we hit a back node make a list of all loops
        ಯದಿ (vertexp->user() == 1) {
            m_graphp->loopsMessageCb(vertexp, m_edgeFuncp);
            ಪ್ರತ್ಯಾಗಮ vertexp->rank();
        }
        ಯದಿ (vertexp->user()) ಪ್ರತ್ಯಾಗಮ vertexp->rank();  // Done earlier
        vertexp->user(1);
        vertexp->rank(1);  // In case loop
        // If no input edges, then rank 1 (+ adder)
        // Otherwise, get minimum from following all inputs.
        uint32_t minrank = ~0U;
        ಪ್ರತಿ (V3GraphEdge& edge : vertexp->inEdges()) {
            ಯದಿ (followEdge(&edge)) {
                ಧ್ರುವ uint32_t nrank = vertexIterate(edge.fromp());
                minrank = std::min(minrank, nrank);
            }
        }
        ಯದಿ (minrank == ~0U) minrank = 0;
        vertexp->rank(minrank + vertexp->rankAdder());
        vertexp->user(2);
        ಪ್ರತ್ಯಾಗಮ vertexp->rank();
    }

ಸಾರ್ವಜನಿಕ:
    GraphAlgRankMin(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        main();
    }
    ~GraphAlgRankMin() = ಸ್ವಭಾವ;
};

ಶೂನ್ಯ V3Graph::rankMin() { rankMin(&V3GraphEdge::followAlwaysTrue); }
ಶೂನ್ಯ V3Graph::rankMin(V3EdgeFuncP edgeFuncp) { GraphAlgRankMin{ಏತತ್, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - report loops
// Changes user()

ವರ್ಗ GraphAlgRLoops final : GraphAlg<> {
    std::vector<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far
    std::vector<string> m_msgs;  // Output messages
    ತಾರ್ಕಿಕ m_done = ಅಸತ್ಯ;  // Exit algorithm

    ಶೂನ್ಯ main(V3GraphVertex* vertexp) {
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        m_graphp->userClearVertices();
        m_callTrace.reserve(100);
        vertexIterate(vertexp, 0);
    }

    ಶೂನ್ಯ vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // When we hit ourself again, return the list of all loops
        ಯದಿ (m_done) ಪ್ರತ್ಯಾಗಮ;

        // Can't just reserve(), unless we modify size() before setting array directly
        ಯಾವತ್ (m_callTrace.size() <= currentRank) m_callTrace.ತಳ್ಳು_back(vertexp);
        m_callTrace[currentRank++] = vertexp;

        ಯದಿ (vertexp->user() == 1) {
            ಪ್ರತಿ (unsigned i = 0; i < currentRank; i++)
                m_msgs.emplace_back(m_graphp->loopsVertexCb(m_callTrace[i]));
            m_done = ಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (vertexp->user() == 2) ಪ್ರತ್ಯಾಗಮ;  // Already processed it
        vertexp->user(1);
        ಪ್ರತಿ (V3GraphEdge& edge : vertexp->outEdges()) {
            ಯದಿ (followEdge(&edge)) vertexIterate(edge.ತುದಿ(), currentRank);
        }
        vertexp->user(2);
    }

ಸಾರ್ವಜನಿಕ:
    GraphAlgRLoops(V3Graph* graphp, V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp)
        : GraphAlg<>{graphp, edgeFuncp} {
        main(vertexp);
    }
    ~GraphAlgRLoops() = ಸ್ವಭಾವ;
    string message() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ std::accumulate(m_msgs.begin(), m_msgs.end(), std::string{""});
    }
};

string V3Graph::reportLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp) {
    ಪ್ರತ್ಯಾಗಮ GraphAlgRLoops{ಏತತ್, edgeFuncp, vertexp}.message();
}

//######################################################################
//######################################################################
// Algorithms - subtrees
// Changes user()

ವರ್ಗ GraphAlgSubtrees final : GraphAlg<> {
    V3Graph* ಧ್ರುವ m_loopGraphp;

    //! Iterate through all connected nodes of a graph with a loop or loops.
    V3GraphVertex* vertexIterateAll(V3GraphVertex* vertexp) {
        ಯದಿ (V3GraphVertex* newVertexp = static_cast<V3GraphVertex*>(vertexp->userp())) {
            ಪ್ರತ್ಯಾಗಮ newVertexp;
        } ಅನ್ಯಥಾ {
            newVertexp = vertexp->clone(m_loopGraphp);
            vertexp->userp(newVertexp);

            ಪ್ರತಿ (V3GraphEdge& edge : vertexp->outEdges()) {
                ಯದಿ (followEdge(&edge)) {
                    V3GraphEdge* newEdgep = static_cast<V3GraphEdge*>(edge.userp());
                    ಯದಿ (!newEdgep) {
                        V3GraphVertex* new_ತುದಿ = vertexIterateAll(edge.ತುದಿ());
                        newEdgep = edge.clone(m_loopGraphp, newVertexp, new_ತುದಿ);
                        edge.userp(newEdgep);
                    }
                }
            }
            ಪ್ರತ್ಯಾಗಮ newVertexp;
        }
    }

ಸಾರ್ವಜನಿಕ:
    GraphAlgSubtrees(V3Graph* graphp, V3Graph* loopGraphp, V3EdgeFuncP edgeFuncp,
                     V3GraphVertex* vertexp)
        : GraphAlg<>{graphp, edgeFuncp}
        , m_loopGraphp{loopGraphp} {
        // Vertex::m_userp - New vertex if we have seen this vertex already
        // Edge::m_userp - New edge if we have seen this edge already
        m_graphp->userClearVertices();
        m_graphp->userClearEdges();
        (ಶೂನ್ಯ)vertexIterateAll(vertexp);
    }
    ~GraphAlgSubtrees() = ಸ್ವಭಾವ;
};

//! Report the entire connected graph with a loop or loops
ಶೂನ್ಯ V3Graph::subtreeLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp, V3Graph* loopGraphp) {
    GraphAlgSubtrees{ಏತತ್, loopGraphp, edgeFuncp, vertexp};
}

//######################################################################
//######################################################################
// Algorithms - sorting

ಶೂನ್ಯ V3Graph::sortVertices() {
    // Sort list of vertices by rank, then fanout
    std::vector<V3GraphVertex*> vertexps;
    ಪ್ರತಿ (V3GraphVertex& vertex : m_vertices) vertexps.ತಳ್ಳು_back(&vertex);
    std::stable_sort(vertexps.begin(), vertexps.end(),
                     [](ಧ್ರುವ V3GraphVertex* lhsp, ಧ್ರುವ V3GraphVertex* rhsp) {  //
                         ಪ್ರತ್ಯಾಗಮ lhsp->sortCmp(rhsp) < 0;
                     });
    // Re-insert in sorted order
    ಪ್ರತಿ (V3GraphVertex* ಧ್ರುವ vertexp : vertexps) {
        m_vertices.unlink(vertexp);
        m_vertices.linkBack(vertexp);
    }
}

ಶೂನ್ಯ V3Graph::sortEdges() {
    // Sort edges by rank then fanout of node they point to
    std::vector<V3GraphEdge*> edges;
    ಪ್ರತಿ (V3GraphVertex& vertex : vertices()) {
        // Make a vector
        ಪ್ರತಿ (V3GraphEdge& edge : vertex.outEdges()) edges.ತಳ್ಳು_back(&edge);
        // Sort
        std::stable_sort(edges.begin(), edges.end(),
                         [](ಧ್ರುವ V3GraphEdge* lhsp, ಧ್ರುವ V3GraphEdge* rhsp) {  //
                             ಪ್ರತ್ಯಾಗಮ lhsp->sortCmp(rhsp) < 0;
                         });
        // Relink edges in specified order
        ಪ್ರತಿ (V3GraphEdge* ಧ್ರುವ edgep : edges) edgep->relinkFromp(&vertex);
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

ಶೂನ್ಯ V3Graph::order() {
    UINFO(2, "Order:");

    // Compute rankings again
    rank(&V3GraphEdge::followAlwaysTrue);
    orderPreRanked();
}

ಶೂನ್ಯ V3Graph::orderPreRanked() {
    // Compute fanouts
    // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
    userClearVertices();
    ಪ್ರತಿ (V3GraphVertex& vertex : vertices()) {
        ಯದಿ (!vertex.user()) orderDFSIterate(&vertex);
    }

    // Sort list of vertices by rank, then fanout. Fanout is a bit of a
    // misnomer. It is the sum of all the fanouts of nodes reached from a node
    // *plus* the count of edges in to that node.
    sortVertices();
    // Sort edges by rank then fanout of node they point to
    sortEdges();
}

ದ್ವಿಗುಣ V3Graph::orderDFSIterate(V3GraphVertex* vertexp) {
    // Compute fanouts of each node
    // If forward edge, don't double count that fanout
    ಯದಿ (vertexp->user() == 2) ಪ್ರತ್ಯಾಗಮ vertexp->fanout();  // Already processed it
    UASSERT_OBJ(vertexp->user() != 1, vertexp, "Loop found, backward edges should be dead");
    vertexp->user(1);
    ದ್ವಿಗುಣ fanout = 0;
    ಪ್ರತಿ (V3GraphEdge& edge : vertexp->outEdges()) {
        ಯದಿ (edge.weight()) fanout += orderDFSIterate(edge.ತುದಿ());
    }
    // Just count inbound edges
    ಪ್ರತಿ (V3GraphEdge& edge : vertexp->inEdges()) {
        ಯದಿ (edge.weight()) ++fanout;
    }
    vertexp->fanout(fanout);
    vertexp->user(2);
    ಪ್ರತ್ಯಾಗಮ vertexp->fanout();
}

//######################################################################
//######################################################################
// Algorithms - parallelism report

ವರ್ಗ GraphAlgParallelismReport final {
    // MEMBERS
    V3Graph& m_graph;  // The graph
    ಧ್ರುವ std::function<uint64_t(ಧ್ರುವ V3GraphVertex*)> m_vertexCost;  // vertex cost function
    V3Graph::ParallelismReport m_report;  // The result report

    // CONSTRUCTORS
    explicit GraphAlgParallelismReport(V3Graph& graph,
                                       std::function<uint64_t(ಧ್ರುವ V3GraphVertex*)> vertexCost)
        : m_graph{graph}
        , m_vertexCost{vertexCost} {
        // For each node, record the critical path cost from the start
        // of the graph through the end of the node.
        std::unordered_map<ಧ್ರುವ V3GraphVertex*, uint64_t> critPaths;
        GraphStreamUnordered serialize{&m_graph};
        ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex* vertexp; (vertexp = serialize.nextp());) {
            ++m_report.m_vertex_ಎಣಿಕೆ;
            uint64_t cpCostToHere = 0;
            ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : vertexp->inEdges()) {
                ++m_report.m_edge_ಎಣಿಕೆ;
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
    ~GraphAlgParallelismReport() = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(GraphAlgParallelismReport);
    VL_UNMOVABLE(GraphAlgParallelismReport);

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ V3Graph::ParallelismReport
    apply(V3Graph& graph, std::function<uint32_t(ಧ್ರುವ V3GraphVertex*)> vertexCost) {
        ಪ್ರತ್ಯಾಗಮ GraphAlgParallelismReport(graph, vertexCost).m_report;
    }
};

V3Graph::ParallelismReport
V3Graph::parallelismReport(std::function<uint64_t(ಧ್ರುವ V3GraphVertex*)> vertexCost) {
    ಪ್ರತ್ಯಾಗಮ GraphAlgParallelismReport::apply(*ಏತತ್, vertexCost);
}
