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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3GraphAlg.h"

#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3GraphPathChecker.h"
#ଅନ୍ତର୍ଭାଵ "V3GraphStream.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <limits>
#ଅନ୍ତର୍ଭାଵ <list>
#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <numeric>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - Remove redundancies
// Changes user() and weight()

ଶ୍ରେଣୀ GraphRemoveRedundant final : GraphAlg<> {
    ଅଚଳ ତାର୍କିକ m_sumWeights;  // Sum, rather then maximize weights
ବ୍ୟକ୍ତିଗତ:
    ଶୂନ୍ୟ ମୁଖ୍ୟ() {
        ପାଇଁ (V3GraphVertex& vertex : m_graphp->vertices()) vertexIterate(&vertex);
    }
    ଶୂନ୍ୟ vertexIterate(V3GraphVertex* vertexp) {
        // Clear marks
        ପାଇଁ (V3GraphEdge& edge : vertexp->outEdges()) edge.ଶୀର୍ଷ()->userp(nullptr);

        // Mark edges and detect duplications

        ପାଇଁ (V3GraphEdge* ଅଚଳ edgep : vertexp->outEdges().unlinkable()) {
            ଯଦି (followEdge(edgep)) {
                V3GraphVertex* outVertexp = edgep->ଶୀର୍ଷ();
                V3GraphEdge* prevEdgep = static_cast<V3GraphEdge*>(outVertexp->userp());
                ଯଦି (!prevEdgep) {  // No previous assignment
                    outVertexp->userp(edgep);
                } ଅନ୍ୟଥା {  // Duplicate
                    ତାର୍କିକ saveOld = ସତ୍ୟ;
                    ଯଦି (prevEdgep->cutable() && !edgep->cutable()) {
                        saveOld = ମିଥ୍ୟା;  // new !cutable more important than old
                    } ଅନ୍ୟଥା ଯଦି (!prevEdgep->cutable() && edgep->cutable()) {
                        saveOld = ସତ୍ୟ;  // old !cutable more important than new
                    } ଅନ୍ୟଥା {
                        saveOld = ସତ୍ୟ;
                        ଯଦି (!m_sumWeights
                            && (prevEdgep->weight() < edgep->weight())) {  // Keep max weight
                            prevEdgep->weight(edgep->weight());
                        }
                    }
                    ଯଦି (saveOld) {
                        ଯଦି (m_sumWeights) prevEdgep->weight(prevEdgep->weight() + edgep->weight());
                        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    } ଅନ୍ୟଥା {
                        ଯଦି (m_sumWeights) edgep->weight(prevEdgep->weight() + edgep->weight());
                        VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                        outVertexp->userp(edgep);
                    }
                }
            }
        }
    }

ସର୍ବସାଧାରଣ:
    GraphRemoveRedundant(V3Graph* graphp, V3EdgeFuncP edgeFuncp, ତାର୍କିକ sumWeights)
        : GraphAlg<>{graphp, edgeFuncp}
        , m_sumWeights{sumWeights} {
        ମୁଖ୍ୟ();
    }
    ~GraphRemoveRedundant() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

ଶୂନ୍ୟ V3Graph::removeRedundantEdgesMax(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{ଏହି, edgeFuncp, ମିଥ୍ୟା};
}
ଶୂନ୍ୟ V3Graph::removeRedundantEdgesSum(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{ଏହି, edgeFuncp, ସତ୍ୟ};
}

//######################################################################
//######################################################################
// Algorithms - remove transitive

ଶ୍ରେଣୀ GraphAlgRemoveTransitiveEdges final : GraphAlg<> {
ସର୍ବସାଧାରଣ:
    explicit GraphAlgRemoveTransitiveEdges(V3Graph* graphp)
        : GraphAlg<>(graphp, nullptr) {}
    ଶୂନ୍ୟ go() {
        GraphPathChecker checker{m_graphp};
        ପାଇଁ (V3GraphVertex& vtx : m_graphp->vertices()) {
            V3GraphEdge* deletep = nullptr;
            ପାଇଁ (V3GraphEdge& edge : vtx.outEdges()) {
                ଯଦି (deletep) VL_DO_CLEAR(deletep->unlinkDelete(), deletep = nullptr);
                // It should be safe to modify the graph, despite using
                // the GraphPathChecker, as none of the modifications will
                // change what can be reached from what, nor should they
                // change the rank or CP of any node.
                ଯଦି (checker.isTransitiveEdge(&edge)) deletep = &edge;
            }
            ଯଦି (deletep) VL_DO_DANGLING(deletep->unlinkDelete(), deletep);
        }
    }

ବ୍ୟକ୍ତିଗତ:
    VL_UNCOPYABLE(GraphAlgRemoveTransitiveEdges);
};

ଶୂନ୍ୟ V3Graph::removeTransitiveEdges() { GraphAlgRemoveTransitiveEdges{ଏହି}.go(); }

//######################################################################
//######################################################################
// Algorithms - weakly connected components
// Changes color()

ଶ୍ରେଣୀ GraphAlgWeakly final : GraphAlg<> {
    // Sentinel color, meaning not colored yet. Colors themselves are 0 .. m_numColors-1.
    ସ୍ଥିର constexpr uint32_t UNCOLORED = std::numeric_limits<uint32_t>::max();

    uint32_t m_ଅଙ୍କ_Colors = 0;  // Number of colors assigned

    ଶୂନ୍ୟ ମୁଖ୍ୟ() {
        // Initialize state
        ପାଇଁ (V3GraphVertex& vertex : m_graphp->vertices()) vertex.color(UNCOLORED);
        // Color graph, without gaps
        ପାଇଁ (V3GraphVertex& vertex : m_graphp->vertices()) {
            ଯଦି (vertex.color() == UNCOLORED) vertexIterate(&vertex, m_ଅଙ୍କ_Colors++);
        }
    }

    ଶୂନ୍ୟ vertexIterate(V3GraphVertex* vertexp, uint32_t currentColor) {
        // Assign new color to each unvisited node
        // then visit each of its edges, giving them the same color
        ଯଦି (vertexp->color() != UNCOLORED) ଫେରନ୍ତୁ;  // Already colored it
        vertexp->color(currentColor);
        ପାଇଁ (V3GraphEdge& edge : vertexp->outEdges()) {
            ଯଦି (followEdge(&edge)) vertexIterate(edge.ଶୀର୍ଷ(), currentColor);
        }
        ପାଇଁ (V3GraphEdge& edge : vertexp->inEdges()) {
            ଯଦି (followEdge(&edge)) vertexIterate(edge.fromp(), currentColor);
        }
    }

ସର୍ବସାଧାରଣ:
    GraphAlgWeakly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>(graphp, edgeFuncp) {
        ମୁଖ୍ୟ();
    }
    ~GraphAlgWeakly() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    uint32_t ଅଙ୍କ_Colors() ଅଚଳ { ଫେରନ୍ତୁ m_ଅଙ୍କ_Colors; }
};

uint32_t V3Graph::weaklyConnected(V3EdgeFuncP edgeFuncp) {
    ଫେରନ୍ତୁ GraphAlgWeakly{ଏହି, edgeFuncp}.ଅଙ୍କ_Colors();
}

//######################################################################
//######################################################################
// Algorithms - strongly connected components
// Changes user() and color()

ଶ୍ରେଣୀ GraphAlgStrongly final : GraphAlg<> {
    uint32_t m_currentDfs = 0;  // DFS count
    std::vector<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far

    ଶୂନ୍ୟ ମୁଖ୍ୟ() {
        // Use Pearce's algorithm to color the strongly connected components. For reference see
        // "An Improved Algorithm for Finding the Strongly Connected Components of a Directed
        // Graph", David J.Pearce, 2005
        //
        // Node State:
        //     Vertex::user     // DFS number indicating possible root of subtree, 0=not iterated
        //     Vertex::color    // Output subtree number (fully processed)

        // Clear info
        ପାଇଁ (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.color(0);
            vertex.user(0);
        }
        // Color graph
        ପାଇଁ (V3GraphVertex& vertex : m_graphp->vertices()) {
            ଯଦି (!vertex.user()) {
                m_currentDfs++;
                vertexIterate(&vertex);
            }
        }
        // If there's a single vertex of a color, it doesn't need a subgraph
        // This simplifies the consumer's code, and reduces graph debugging clutter
        ପାଇଁ (V3GraphVertex& vertex : m_graphp->vertices()) {
            ତାର୍କିକ onecolor = ସତ୍ୟ;
            ପାଇଁ (V3GraphEdge& edge : vertex.outEdges()) {
                ଯଦି (followEdge(&edge)) {
                    ଯଦି (vertex.color() == edge.ଶୀର୍ଷ()->color()) {
                        onecolor = ମିଥ୍ୟା;
                        ଭାଙ୍ଗ;
                    }
                }
            }
            ଯଦି (onecolor) vertex.color(0);
        }
    }

    ଶୂନ୍ୟ vertexIterate(V3GraphVertex* vertexp) {
        ଅଚଳ uint32_t this_Dfs_ଅଙ୍କ = m_currentDfs++;
        vertexp->user(this_Dfs_ଅଙ୍କ);
        vertexp->color(0);
        ପାଇଁ (V3GraphEdge& edge : vertexp->outEdges()) {
            ଯଦି (followEdge(&edge)) {
                V3GraphVertex* ଶୀର୍ଷ = edge.ଶୀର୍ଷ();
                ଯଦି (!ଶୀର୍ଷ->user()) {  // Dest not computed yet
                    vertexIterate(ଶୀର୍ଷ);
                }
                ଯଦି (!ଶୀର୍ଷ->color()) {  // Dest not in a component
                    ଯଦି (vertexp->user() > ଶୀର୍ଷ->user()) vertexp->user(ଶୀର୍ଷ->user());
                }
            }
        }
        ଯଦି (vertexp->user() == this_Dfs_ଅଙ୍କ) {  // New head of subtree
            vertexp->color(this_Dfs_ଅଙ୍କ);  // Mark as component
            ଯେତେବେଳେ (!m_callTrace.empty()) {
                V3GraphVertex* କାଢ଼_Vertexp = m_callTrace.back();
                ଯଦି (କାଢ଼_Vertexp->user() >= this_Dfs_ଅଙ୍କ) {  // Lower node is part of this subtree
                    m_callTrace.କାଢ଼_back();
                    କାଢ଼_Vertexp->color(this_Dfs_ଅଙ୍କ);
                } ଅନ୍ୟଥା {
                    ଭାଙ୍ଗ;
                }
            }
        } ଅନ୍ୟଥା {  // In another subtree (maybe...)
            m_callTrace.ଠେଲ_back(vertexp);
        }
    }

ସର୍ବସାଧାରଣ:
    GraphAlgStrongly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        ମୁଖ୍ୟ();
    }
    ~GraphAlgStrongly() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

ଶୂନ୍ୟ V3Graph::stronglyConnected(V3EdgeFuncP edgeFuncp) { GraphAlgStrongly{ଏହି, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking
// Changes user() and rank()

ଶ୍ରେଣୀ GraphAlgRank final : GraphAlg<> {
    ଶୂନ୍ୟ ମୁଖ୍ୟ() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        ପାଇଁ (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        ପାଇଁ (V3GraphVertex& vertex : m_graphp->vertices()) {
            ଯଦି (!vertex.user()) vertexIterate(&vertex, 1);
        }
    }
    ଶୂନ୍ୟ vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // If larger rank is found, assign it and loop back through
        // If we hit a back node make a list of all loops
        ଯଦି (vertexp->user() == 1) {
            m_graphp->loopsMessageCb(vertexp, m_edgeFuncp);
            ଫେରନ୍ତୁ;  // LCOV_EXCL_LINE  // gcc gprof bug misses this return
        }
        ଯଦି (vertexp->rank() >= currentRank) ଫେରନ୍ତୁ;  // Already processed it
        vertexp->user(1);
        vertexp->rank(currentRank);
        ପାଇଁ (V3GraphEdge& edge : vertexp->outEdges()) {
            ଯଦି (followEdge(&edge)) vertexIterate(edge.ଶୀର୍ଷ(), currentRank + vertexp->rankAdder());
        }
        vertexp->user(2);
    }

ସର୍ବସାଧାରଣ:
    GraphAlgRank(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        ମୁଖ୍ୟ();
    }
    ~GraphAlgRank() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

ଶୂନ୍ୟ V3Graph::rank() { rank(&V3GraphEdge::followAlwaysTrue); }
ଶୂନ୍ୟ V3Graph::rank(V3EdgeFuncP edgeFuncp) { GraphAlgRank{ଏହି, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking min
// Changes user() and rank()

ଶ୍ରେଣୀ GraphAlgRankMin final : GraphAlg<> {
    ଶୂନ୍ୟ ମୁଖ୍ୟ() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        ପାଇଁ (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        ପାଇଁ (V3GraphVertex& vertex : m_graphp->vertices()) {
            ଯଦି (!vertex.user()) vertexIterate(&vertex);
        }
    }
    uint32_t vertexIterate(V3GraphVertex* vertexp) {
        // Assign rank to each unvisited node
        // If we hit a back node make a list of all loops
        ଯଦି (vertexp->user() == 1) {
            m_graphp->loopsMessageCb(vertexp, m_edgeFuncp);
            ଫେରନ୍ତୁ vertexp->rank();
        }
        ଯଦି (vertexp->user()) ଫେରନ୍ତୁ vertexp->rank();  // Done earlier
        vertexp->user(1);
        vertexp->rank(1);  // In case loop
        // If no input edges, then rank 1 (+ adder)
        // Otherwise, get minimum from following all inputs.
        uint32_t minrank = ~0U;
        ପାଇଁ (V3GraphEdge& edge : vertexp->inEdges()) {
            ଯଦି (followEdge(&edge)) {
                ଅଚଳ uint32_t nrank = vertexIterate(edge.fromp());
                minrank = std::min(minrank, nrank);
            }
        }
        ଯଦି (minrank == ~0U) minrank = 0;
        vertexp->rank(minrank + vertexp->rankAdder());
        vertexp->user(2);
        ଫେରନ୍ତୁ vertexp->rank();
    }

ସର୍ବସାଧାରଣ:
    GraphAlgRankMin(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        ମୁଖ୍ୟ();
    }
    ~GraphAlgRankMin() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

ଶୂନ୍ୟ V3Graph::rankMin() { rankMin(&V3GraphEdge::followAlwaysTrue); }
ଶୂନ୍ୟ V3Graph::rankMin(V3EdgeFuncP edgeFuncp) { GraphAlgRankMin{ଏହି, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - report loops
// Changes user()

ଶ୍ରେଣୀ GraphAlgRLoops final : GraphAlg<> {
    std::vector<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far
    std::vector<string> m_msgs;  // Output messages
    ତାର୍କିକ m_done = ମିଥ୍ୟା;  // Exit algorithm

    ଶୂନ୍ୟ ମୁଖ୍ୟ(V3GraphVertex* vertexp) {
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        m_graphp->userClearVertices();
        m_callTrace.reserve(100);
        vertexIterate(vertexp, 0);
    }

    ଶୂନ୍ୟ vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // When we hit ourself again, return the list of all loops
        ଯଦି (m_done) ଫେରନ୍ତୁ;

        // Can't just reserve(), unless we modify size() before setting array directly
        ଯେତେବେଳେ (m_callTrace.size() <= currentRank) m_callTrace.ଠେଲ_back(vertexp);
        m_callTrace[currentRank++] = vertexp;

        ଯଦି (vertexp->user() == 1) {
            ପାଇଁ (unsigned i = 0; i < currentRank; i++)
                m_msgs.emplace_back(m_graphp->loopsVertexCb(m_callTrace[i]));
            m_done = ସତ୍ୟ;
            ଫେରନ୍ତୁ;
        }
        ଯଦି (vertexp->user() == 2) ଫେରନ୍ତୁ;  // Already processed it
        vertexp->user(1);
        ପାଇଁ (V3GraphEdge& edge : vertexp->outEdges()) {
            ଯଦି (followEdge(&edge)) vertexIterate(edge.ଶୀର୍ଷ(), currentRank);
        }
        vertexp->user(2);
    }

ସର୍ବସାଧାରଣ:
    GraphAlgRLoops(V3Graph* graphp, V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp)
        : GraphAlg<>{graphp, edgeFuncp} {
        ମୁଖ୍ୟ(vertexp);
    }
    ~GraphAlgRLoops() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    string message() ଅଚଳ {
        ଫେରନ୍ତୁ std::accumulate(m_msgs.begin(), m_msgs.end(), std::string{""});
    }
};

string V3Graph::reportLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp) {
    ଫେରନ୍ତୁ GraphAlgRLoops{ଏହି, edgeFuncp, vertexp}.message();
}

//######################################################################
//######################################################################
// Algorithms - subtrees
// Changes user()

ଶ୍ରେଣୀ GraphAlgSubtrees final : GraphAlg<> {
    V3Graph* ଅଚଳ m_loopGraphp;

    //! Iterate through all connected nodes of a graph with a loop or loops.
    V3GraphVertex* vertexIterateAll(V3GraphVertex* vertexp) {
        ଯଦି (V3GraphVertex* newVertexp = static_cast<V3GraphVertex*>(vertexp->userp())) {
            ଫେରନ୍ତୁ newVertexp;
        } ଅନ୍ୟଥା {
            newVertexp = vertexp->clone(m_loopGraphp);
            vertexp->userp(newVertexp);

            ପାଇଁ (V3GraphEdge& edge : vertexp->outEdges()) {
                ଯଦି (followEdge(&edge)) {
                    V3GraphEdge* newEdgep = static_cast<V3GraphEdge*>(edge.userp());
                    ଯଦି (!newEdgep) {
                        V3GraphVertex* new_ଶୀର୍ଷ = vertexIterateAll(edge.ଶୀର୍ଷ());
                        newEdgep = edge.clone(m_loopGraphp, newVertexp, new_ଶୀର୍ଷ);
                        edge.userp(newEdgep);
                    }
                }
            }
            ଫେରନ୍ତୁ newVertexp;
        }
    }

ସର୍ବସାଧାରଣ:
    GraphAlgSubtrees(V3Graph* graphp, V3Graph* loopGraphp, V3EdgeFuncP edgeFuncp,
                     V3GraphVertex* vertexp)
        : GraphAlg<>{graphp, edgeFuncp}
        , m_loopGraphp{loopGraphp} {
        // Vertex::m_userp - New vertex if we have seen this vertex already
        // Edge::m_userp - New edge if we have seen this edge already
        m_graphp->userClearVertices();
        m_graphp->userClearEdges();
        (ଶୂନ୍ୟ)vertexIterateAll(vertexp);
    }
    ~GraphAlgSubtrees() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//! Report the entire connected graph with a loop or loops
ଶୂନ୍ୟ V3Graph::subtreeLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp, V3Graph* loopGraphp) {
    GraphAlgSubtrees{ଏହି, loopGraphp, edgeFuncp, vertexp};
}

//######################################################################
//######################################################################
// Algorithms - sorting

ଶୂନ୍ୟ V3Graph::sortVertices() {
    // Sort list of vertices by rank, then fanout
    std::vector<V3GraphVertex*> vertexps;
    ପାଇଁ (V3GraphVertex& vertex : m_vertices) vertexps.ଠେଲ_back(&vertex);
    std::stable_sort(vertexps.begin(), vertexps.end(),
                     [](ଅଚଳ V3GraphVertex* lhsp, ଅଚଳ V3GraphVertex* rhsp) {  //
                         ଫେରନ୍ତୁ lhsp->sortCmp(rhsp) < 0;
                     });
    // Re-insert in sorted order
    ପାଇଁ (V3GraphVertex* ଅଚଳ vertexp : vertexps) {
        m_vertices.unlink(vertexp);
        m_vertices.linkBack(vertexp);
    }
}

ଶୂନ୍ୟ V3Graph::sortEdges() {
    // Sort edges by rank then fanout of node they point to
    std::vector<V3GraphEdge*> edges;
    ପାଇଁ (V3GraphVertex& vertex : vertices()) {
        // Make a vector
        ପାଇଁ (V3GraphEdge& edge : vertex.outEdges()) edges.ଠେଲ_back(&edge);
        // Sort
        std::stable_sort(edges.begin(), edges.end(),
                         [](ଅଚଳ V3GraphEdge* lhsp, ଅଚଳ V3GraphEdge* rhsp) {  //
                             ଫେରନ୍ତୁ lhsp->sortCmp(rhsp) < 0;
                         });
        // Relink edges in specified order
        ପାଇଁ (V3GraphEdge* ଅଚଳ edgep : edges) edgep->relinkFromp(&vertex);
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

ଶୂନ୍ୟ V3Graph::order() {
    UINFO(2, "Order:");

    // Compute rankings again
    rank(&V3GraphEdge::followAlwaysTrue);
    orderPreRanked();
}

ଶୂନ୍ୟ V3Graph::orderPreRanked() {
    // Compute fanouts
    // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
    userClearVertices();
    ପାଇଁ (V3GraphVertex& vertex : vertices()) {
        ଯଦି (!vertex.user()) orderDFSIterate(&vertex);
    }

    // Sort list of vertices by rank, then fanout. Fanout is a bit of a
    // misnomer. It is the sum of all the fanouts of nodes reached from a node
    // *plus* the count of edges in to that node.
    sortVertices();
    // Sort edges by rank then fanout of node they point to
    sortEdges();
}

ଦ୍ୱିଗୁଣ V3Graph::orderDFSIterate(V3GraphVertex* vertexp) {
    // Compute fanouts of each node
    // If forward edge, don't double count that fanout
    ଯଦି (vertexp->user() == 2) ଫେରନ୍ତୁ vertexp->fanout();  // Already processed it
    UASSERT_OBJ(vertexp->user() != 1, vertexp, "Loop found, backward edges should be dead");
    vertexp->user(1);
    ଦ୍ୱିଗୁଣ fanout = 0;
    ପାଇଁ (V3GraphEdge& edge : vertexp->outEdges()) {
        ଯଦି (edge.weight()) fanout += orderDFSIterate(edge.ଶୀର୍ଷ());
    }
    // Just count inbound edges
    ପାଇଁ (V3GraphEdge& edge : vertexp->inEdges()) {
        ଯଦି (edge.weight()) ++fanout;
    }
    vertexp->fanout(fanout);
    vertexp->user(2);
    ଫେରନ୍ତୁ vertexp->fanout();
}

//######################################################################
//######################################################################
// Algorithms - parallelism report

ଶ୍ରେଣୀ GraphAlgParallelismReport final {
    // MEMBERS
    V3Graph& m_graph;  // The graph
    ଅଚଳ std::function<uint64_t(ଅଚଳ V3GraphVertex*)> m_vertexCost;  // vertex cost function
    V3Graph::ParallelismReport m_report;  // The result report

    // CONSTRUCTORS
    explicit GraphAlgParallelismReport(V3Graph& graph,
                                       std::function<uint64_t(ଅଚଳ V3GraphVertex*)> vertexCost)
        : m_graph{graph}
        , m_vertexCost{vertexCost} {
        // For each node, record the critical path cost from the start
        // of the graph through the end of the node.
        std::unordered_map<ଅଚଳ V3GraphVertex*, uint64_t> critPaths;
        GraphStreamUnordered serialize{&m_graph};
        ପାଇଁ (ଅଚଳ V3GraphVertex* vertexp; (vertexp = serialize.nextp());) {
            ++m_report.m_vertex_ଗଣନା;
            uint64_t cpCostToHere = 0;
            ପାଇଁ (ଅଚଳ V3GraphEdge& edge : vertexp->inEdges()) {
                ++m_report.m_edge_ଗଣନା;
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
    ~GraphAlgParallelismReport() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(GraphAlgParallelismReport);
    VL_UNMOVABLE(GraphAlgParallelismReport);

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର V3Graph::ParallelismReport
    apply(V3Graph& graph, std::function<uint32_t(ଅଚଳ V3GraphVertex*)> vertexCost) {
        ଫେରନ୍ତୁ GraphAlgParallelismReport(graph, vertexCost).m_report;
    }
};

V3Graph::ParallelismReport
V3Graph::parallelismReport(std::function<uint64_t(ଅଚଳ V3GraphVertex*)> vertexCost) {
    ଫେରନ୍ତୁ GraphAlgParallelismReport::apply(*ଏହି, vertexCost);
}
