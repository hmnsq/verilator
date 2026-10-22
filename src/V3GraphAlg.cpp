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

#परिभाषय VL_MT_DISABLED_संहिता_UNIT 1

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
    ध्रुव तार्किक m_योग_Weights;  // Sum, rather then maximize weights
गुप्त:
    शून्य मुख्यम्() {
        प्रति (V3GraphVertex& vertex : m_graphp->vertices()) vertexIterate(&vertex);
    }
    शून्य vertexIterate(V3GraphVertex* vertexp) {
        // Clear marks
        प्रति (V3GraphEdge& edge : vertexp->बहिर्गमनम्_धाराः()) edge.शिखरम्()->userp(शून्यसूचकः);

        // Mark edges and detect duplications

        प्रति (V3GraphEdge* ध्रुव edgep : vertexp->बहिर्गमनम्_धाराः().unlinkable()) {
            यदि (followEdge(edgep)) {
                V3GraphVertex* बहिर्गमनम्_Vertexp = edgep->शिखरम्();
                V3GraphEdge* पूर्व_Edgep = static_cast<V3GraphEdge*>(बहिर्गमनम्_Vertexp->userp());
                यदि (!पूर्व_Edgep) {  // No previous assignment
                    बहिर्गमनम्_Vertexp->userp(edgep);
                } अन्यथा {  // Duplicate
                    तार्किक saveOld = सत्य;
                    यदि (पूर्व_Edgep->cutable() && !edgep->cutable()) {
                        saveOld = असत्य;  // new !cutable more important than old
                    } अन्यथा यदि (!पूर्व_Edgep->cutable() && edgep->cutable()) {
                        saveOld = सत्य;  // old !cutable more important than new
                    } अन्यथा {
                        saveOld = सत्य;
                        यदि (!m_योग_Weights
                            && (पूर्व_Edgep->weight() < edgep->weight())) {  // Keep max weight
                            पूर्व_Edgep->weight(edgep->weight());
                        }
                    }
                    यदि (saveOld) {
                        यदि (m_योग_Weights) पूर्व_Edgep->weight(पूर्व_Edgep->weight() + edgep->weight());
                        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    } अन्यथा {
                        यदि (m_योग_Weights) edgep->weight(पूर्व_Edgep->weight() + edgep->weight());
                        VL_DO_DANGLING(पूर्व_Edgep->unlinkDelete(), पूर्व_Edgep);
                        बहिर्गमनम्_Vertexp->userp(edgep);
                    }
                }
            }
        }
    }

सार्वजनिक:
    GraphRemoveRedundant(V3Graph* graphp, V3EdgeFuncP edgeFuncp, तार्किक योग_Weights)
        : GraphAlg<>{graphp, edgeFuncp}
        , m_योग_Weights{योग_Weights} {
        मुख्यम्();
    }
    ~GraphRemoveRedundant() = स्वभाव;
};

शून्य V3Graph::remove_Redundant_धाराः_Max(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{एतत्, edgeFuncp, असत्य};
}
शून्य V3Graph::remove_Redundant_धाराः_योग(V3EdgeFuncP edgeFuncp) {
    GraphRemoveRedundant{एतत्, edgeFuncp, सत्य};
}

//######################################################################
//######################################################################
// Algorithms - remove transitive

वर्ग Graph_Alg_Remove_संक्रामकम्_धाराः final : GraphAlg<> {
सार्वजनिक:
    स्पष्ट Graph_Alg_Remove_संक्रामकम्_धाराः(V3Graph* graphp)
        : GraphAlg<>(graphp, शून्यसूचकः) {}
    शून्य go() {
        Graph_मार्गः_Checker checker{m_graphp};
        प्रति (V3GraphVertex& vtx : m_graphp->vertices()) {
            V3GraphEdge* deletep = शून्यसूचकः;
            प्रति (V3GraphEdge& edge : vtx.बहिर्गमनम्_धाराः()) {
                यदि (deletep) VL_DO_CLEAR(deletep->unlinkDelete(), deletep = शून्यसूचकः);
                // It should be safe to modify the graph, despite using
                // the GraphPathChecker, as none of the modifications will
                // change what can be reached from what, nor should they
                // change the rank or CP of any node.
                यदि (checker.is_संक्रामकम्_Edge(&edge)) deletep = &edge;
            }
            यदि (deletep) VL_DO_DANGLING(deletep->unlinkDelete(), deletep);
        }
    }

गुप्त:
    VL_UNCOPYABLE(Graph_Alg_Remove_संक्रामकम्_धाराः);
};

शून्य V3Graph::remove_संक्रामकम्_धाराः() { Graph_Alg_Remove_संक्रामकम्_धाराः{एतत्}.go(); }

//######################################################################
//######################################################################
// Algorithms - weakly connected components
// Changes color()

वर्ग GraphAlgWeakly final : GraphAlg<> {
    // Sentinel color, meaning not colored yet. Colors themselves are 0 .. m_numColors-1.
    स्थिर constexpr uint32_t UNCOLORED = std::numeric_limits<uint32_t>::max();

    uint32_t m_अङ्कः_Colors = 0;  // Number of colors assigned

    शून्य मुख्यम्() {
        // Initialize state
        प्रति (V3GraphVertex& vertex : m_graphp->vertices()) vertex.color(UNCOLORED);
        // Color graph, without gaps
        प्रति (V3GraphVertex& vertex : m_graphp->vertices()) {
            यदि (vertex.color() == UNCOLORED) vertexIterate(&vertex, m_अङ्कः_Colors++);
        }
    }

    शून्य vertexIterate(V3GraphVertex* vertexp, uint32_t currentColor) {
        // Assign new color to each unvisited node
        // then visit each of its edges, giving them the same color
        यदि (vertexp->color() != UNCOLORED) प्रत्यागम;  // Already colored it
        vertexp->color(currentColor);
        प्रति (V3GraphEdge& edge : vertexp->बहिर्गमनम्_धाराः()) {
            यदि (followEdge(&edge)) vertexIterate(edge.शिखरम्(), currentColor);
        }
        प्रति (V3GraphEdge& edge : vertexp->अन्तर्_धाराः()) {
            यदि (followEdge(&edge)) vertexIterate(edge.fromp(), currentColor);
        }
    }

सार्वजनिक:
    GraphAlgWeakly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>(graphp, edgeFuncp) {
        मुख्यम्();
    }
    ~GraphAlgWeakly() = स्वभाव;

    uint32_t अङ्कः_Colors() ध्रुव { प्रत्यागम m_अङ्कः_Colors; }
};

uint32_t V3Graph::weaklyConnected(V3EdgeFuncP edgeFuncp) {
    प्रत्यागम GraphAlgWeakly{एतत्, edgeFuncp}.अङ्कः_Colors();
}

//######################################################################
//######################################################################
// Algorithms - strongly connected components
// Changes user() and color()

वर्ग GraphAlgStrongly final : GraphAlg<> {
    uint32_t m_currentDfs = 0;  // DFS count
    std::सदिशम्<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far

    शून्य मुख्यम्() {
        // Use Pearce's algorithm to color the strongly connected components. For reference see
        // "An Improved Algorithm for Finding the Strongly Connected Components of a Directed
        // Graph", David J.Pearce, 2005
        //
        // Node State:
        //     Vertex::user     // DFS number indicating possible root of subtree, 0=not iterated
        //     Vertex::color    // Output subtree number (fully processed)

        // Clear info
        प्रति (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.color(0);
            vertex.user(0);
        }
        // Color graph
        प्रति (V3GraphVertex& vertex : m_graphp->vertices()) {
            यदि (!vertex.user()) {
                m_currentDfs++;
                vertexIterate(&vertex);
            }
        }
        // If there's a single vertex of a color, it doesn't need a subgraph
        // This simplifies the consumer's code, and reduces graph debugging clutter
        प्रति (V3GraphVertex& vertex : m_graphp->vertices()) {
            तार्किक onecolor = सत्य;
            प्रति (V3GraphEdge& edge : vertex.बहिर्गमनम्_धाराः()) {
                यदि (followEdge(&edge)) {
                    यदि (vertex.color() == edge.शिखरम्()->color()) {
                        onecolor = असत्य;
                        भङ्ग;
                    }
                }
            }
            यदि (onecolor) vertex.color(0);
        }
    }

    शून्य vertexIterate(V3GraphVertex* vertexp) {
        ध्रुव uint32_t this_Dfs_अङ्कः = m_currentDfs++;
        vertexp->user(this_Dfs_अङ्कः);
        vertexp->color(0);
        प्रति (V3GraphEdge& edge : vertexp->बहिर्गमनम्_धाराः()) {
            यदि (followEdge(&edge)) {
                V3GraphVertex* शिखरम् = edge.शिखरम्();
                यदि (!शिखरम्->user()) {  // Dest not computed yet
                    vertexIterate(शिखरम्);
                }
                यदि (!शिखरम्->color()) {  // Dest not in a component
                    यदि (vertexp->user() > शिखरम्->user()) vertexp->user(शिखरम्->user());
                }
            }
        }
        यदि (vertexp->user() == this_Dfs_अङ्कः) {  // New head of subtree
            vertexp->color(this_Dfs_अङ्कः);  // Mark as component
            यावत् (!m_callTrace.रिक्तानि()) {
                V3GraphVertex* निष्कासय_Vertexp = m_callTrace.पृष्ठभागः();
                यदि (निष्कासय_Vertexp->user() >= this_Dfs_अङ्कः) {  // Lower node is part of this subtree
                    m_callTrace.निष्कासय_पृष्ठभागः();
                    निष्कासय_Vertexp->color(this_Dfs_अङ्कः);
                } अन्यथा {
                    भङ्ग;
                }
            }
        } अन्यथा {  // In another subtree (maybe...)
            m_callTrace.प्रक्षिप_पृष्ठभागः(vertexp);
        }
    }

सार्वजनिक:
    GraphAlgStrongly(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        मुख्यम्();
    }
    ~GraphAlgStrongly() = स्वभाव;
};

शून्य V3Graph::stronglyConnected(V3EdgeFuncP edgeFuncp) { GraphAlgStrongly{एतत्, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking
// Changes user() and rank()

वर्ग GraphAlgRank final : GraphAlg<> {
    शून्य मुख्यम्() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        प्रति (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        प्रति (V3GraphVertex& vertex : m_graphp->vertices()) {
            यदि (!vertex.user()) vertexIterate(&vertex, 1);
        }
    }
    शून्य vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // If larger rank is found, assign it and loop back through
        // If we hit a back node make a list of all loops
        यदि (vertexp->user() == 1) {
            m_graphp->loops_सन्देशः_Cb(vertexp, m_edgeFuncp);
            प्रत्यागम;  // LCOV_EXCL_LINE  // gcc gprof bug misses this return
        }
        यदि (vertexp->rank() >= currentRank) प्रत्यागम;  // Already processed it
        vertexp->user(1);
        vertexp->rank(currentRank);
        प्रति (V3GraphEdge& edge : vertexp->बहिर्गमनम्_धाराः()) {
            यदि (followEdge(&edge)) vertexIterate(edge.शिखरम्(), currentRank + vertexp->rankAdder());
        }
        vertexp->user(2);
    }

सार्वजनिक:
    GraphAlgRank(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        मुख्यम्();
    }
    ~GraphAlgRank() = स्वभाव;
};

शून्य V3Graph::rank() { rank(&V3GraphEdge::followAlwaysTrue); }
शून्य V3Graph::rank(V3EdgeFuncP edgeFuncp) { GraphAlgRank{एतत्, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - ranking min
// Changes user() and rank()

वर्ग GraphAlgRankMin final : GraphAlg<> {
    शून्य मुख्यम्() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        प्रति (V3GraphVertex& vertex : m_graphp->vertices()) {
            vertex.rank(0);
            vertex.user(0);
        }
        प्रति (V3GraphVertex& vertex : m_graphp->vertices()) {
            यदि (!vertex.user()) vertexIterate(&vertex);
        }
    }
    uint32_t vertexIterate(V3GraphVertex* vertexp) {
        // Assign rank to each unvisited node
        // If we hit a back node make a list of all loops
        यदि (vertexp->user() == 1) {
            m_graphp->loops_सन्देशः_Cb(vertexp, m_edgeFuncp);
            प्रत्यागम vertexp->rank();
        }
        यदि (vertexp->user()) प्रत्यागम vertexp->rank();  // Done earlier
        vertexp->user(1);
        vertexp->rank(1);  // In case loop
        // If no input edges, then rank 1 (+ adder)
        // Otherwise, get minimum from following all inputs.
        uint32_t minrank = ~0U;
        प्रति (V3GraphEdge& edge : vertexp->अन्तर्_धाराः()) {
            यदि (followEdge(&edge)) {
                ध्रुव uint32_t nrank = vertexIterate(edge.fromp());
                minrank = std::min(minrank, nrank);
            }
        }
        यदि (minrank == ~0U) minrank = 0;
        vertexp->rank(minrank + vertexp->rankAdder());
        vertexp->user(2);
        प्रत्यागम vertexp->rank();
    }

सार्वजनिक:
    GraphAlgRankMin(V3Graph* graphp, V3EdgeFuncP edgeFuncp)
        : GraphAlg<>{graphp, edgeFuncp} {
        मुख्यम्();
    }
    ~GraphAlgRankMin() = स्वभाव;
};

शून्य V3Graph::rankMin() { rankMin(&V3GraphEdge::followAlwaysTrue); }
शून्य V3Graph::rankMin(V3EdgeFuncP edgeFuncp) { GraphAlgRankMin{एतत्, edgeFuncp}; }

//######################################################################
//######################################################################
// Algorithms - report loops
// Changes user()

वर्ग GraphAlgRLoops final : GraphAlg<> {
    std::सदिशम्<V3GraphVertex*> m_callTrace;  // List of everything we hit processing so far
    std::सदिशम्<string> m_msgs;  // Output messages
    तार्किक m_समाप्तम् = असत्य;  // Exit algorithm

    शून्य मुख्यम्(V3GraphVertex* vertexp) {
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        m_graphp->userClearVertices();
        m_callTrace.reserve(100);
        vertexIterate(vertexp, 0);
    }

    शून्य vertexIterate(V3GraphVertex* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // When we hit ourself again, return the list of all loops
        यदि (m_समाप्तम्) प्रत्यागम;

        // Can't just reserve(), unless we modify size() before setting array directly
        यावत् (m_callTrace.आकार() <= currentRank) m_callTrace.प्रक्षिप_पृष्ठभागः(vertexp);
        m_callTrace[currentRank++] = vertexp;

        यदि (vertexp->user() == 1) {
            प्रति (अचिह्नित i = 0; i < currentRank; i++)
                m_msgs.emplace_पृष्ठभागः(m_graphp->loopsVertexCb(m_callTrace[i]));
            m_समाप्तम् = सत्य;
            प्रत्यागम;
        }
        यदि (vertexp->user() == 2) प्रत्यागम;  // Already processed it
        vertexp->user(1);
        प्रति (V3GraphEdge& edge : vertexp->बहिर्गमनम्_धाराः()) {
            यदि (followEdge(&edge)) vertexIterate(edge.शिखरम्(), currentRank);
        }
        vertexp->user(2);
    }

सार्वजनिक:
    GraphAlgRLoops(V3Graph* graphp, V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp)
        : GraphAlg<>{graphp, edgeFuncp} {
        मुख्यम्(vertexp);
    }
    ~GraphAlgRLoops() = स्वभाव;
    string सन्देशः() ध्रुव {
        प्रत्यागम std::accumulate(m_msgs.begin(), m_msgs.अन्तः_2(), std::string{""});
    }
};

string V3Graph::reportLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp) {
    प्रत्यागम GraphAlgRLoops{एतत्, edgeFuncp, vertexp}.सन्देशः();
}

//######################################################################
//######################################################################
// Algorithms - subtrees
// Changes user()

वर्ग GraphAlgSubtrees final : GraphAlg<> {
    V3Graph* ध्रुव m_loopGraphp;

    //! Iterate through all connected nodes of a graph with a loop or loops.
    V3GraphVertex* vertexIterateAll(V3GraphVertex* vertexp) {
        यदि (V3GraphVertex* newVertexp = static_cast<V3GraphVertex*>(vertexp->userp())) {
            प्रत्यागम newVertexp;
        } अन्यथा {
            newVertexp = vertexp->clone(m_loopGraphp);
            vertexp->userp(newVertexp);

            प्रति (V3GraphEdge& edge : vertexp->बहिर्गमनम्_धाराः()) {
                यदि (followEdge(&edge)) {
                    V3GraphEdge* newEdgep = static_cast<V3GraphEdge*>(edge.userp());
                    यदि (!newEdgep) {
                        V3GraphVertex* new_शिखरम् = vertexIterateAll(edge.शिखरम्());
                        newEdgep = edge.clone(m_loopGraphp, newVertexp, new_शिखरम्);
                        edge.userp(newEdgep);
                    }
                }
            }
            प्रत्यागम newVertexp;
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
        m_graphp->user_Clear_धाराः();
        (शून्य)vertexIterateAll(vertexp);
    }
    ~GraphAlgSubtrees() = स्वभाव;
};

//! Report the entire connected graph with a loop or loops
शून्य V3Graph::subtreeLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp, V3Graph* loopGraphp) {
    GraphAlgSubtrees{एतत्, loopGraphp, edgeFuncp, vertexp};
}

//######################################################################
//######################################################################
// Algorithms - sorting

शून्य V3Graph::क्रमय_Vertices() {
    // Sort list of vertices by rank, then fanout
    std::सदिशम्<V3GraphVertex*> vertexps;
    प्रति (V3GraphVertex& vertex : m_vertices) vertexps.प्रक्षिप_पृष्ठभागः(&vertex);
    std::stable_क्रमय(vertexps.begin(), vertexps.अन्तः_2(),
                     [](ध्रुव V3GraphVertex* lhsp, ध्रुव V3GraphVertex* rhsp) {  //
                         प्रत्यागम lhsp->क्रमय_Cmp(rhsp) < 0;
                     });
    // Re-insert in sorted order
    प्रति (V3GraphVertex* ध्रुव vertexp : vertexps) {
        m_vertices.unlink(vertexp);
        m_vertices.link_पृष्ठभागः(vertexp);
    }
}

शून्य V3Graph::क्रमय_धाराः() {
    // Sort edges by rank then fanout of node they point to
    std::सदिशम्<V3GraphEdge*> धाराः;
    प्रति (V3GraphVertex& vertex : vertices()) {
        // Make a vector
        प्रति (V3GraphEdge& edge : vertex.बहिर्गमनम्_धाराः()) धाराः.प्रक्षिप_पृष्ठभागः(&edge);
        //क्रमय
        std::stable_क्रमय(धाराः.begin(), धाराः.अन्तः_2(),
                         [](ध्रुव V3GraphEdge* lhsp, ध्रुव V3GraphEdge* rhsp) {  //
                             प्रत्यागम lhsp->क्रमय_Cmp(rhsp) < 0;
                         });
        // Relink edges in specified order
        प्रति (V3GraphEdge* ध्रुव edgep : धाराः) edgep->relinkFromp(&vertex);
        // Prep for next
        धाराः.clear();
    }
}

//######################################################################
//######################################################################
// Algorithms - ordering
//      Compute near optimal ordering of the nodes, where:
//          If a required    edge is A->B, rank(A)<rank(B)
//          Visit edges and assign ranks to keep minimal crossings
//              (Results in better dcache packing.)

शून्य V3Graph::क्रमः() {
    UINFO(2, "Order:");

    // Compute rankings again
    rank(&V3GraphEdge::followAlwaysTrue);
    क्रमः_Pre_Ranked();
}

शून्य V3Graph::क्रमः_Pre_Ranked() {
    // Compute fanouts
    // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
    userClearVertices();
    प्रति (V3GraphVertex& vertex : vertices()) {
        यदि (!vertex.user()) क्रमः_DFSIterate(&vertex);
    }

    // Sort list of vertices by rank, then fanout. Fanout is a bit of a
    // misnomer. It is the sum of all the fanouts of nodes reached from a node
    // *plus* the count of edges in to that node.
    क्रमय_Vertices();
    // Sort edges by rank then fanout of node they point to
    क्रमय_धाराः();
}

द्विगुण V3Graph::क्रमः_DFSIterate(V3GraphVertex* vertexp) {
    // Compute fanouts of each node
    // If forward edge, don't double count that fanout
    यदि (vertexp->user() == 2) प्रत्यागम vertexp->fanout();  // Already processed it
    UASSERT_OBJ(vertexp->user() != 1, vertexp, "Loop found, backward edges should be dead");
    vertexp->user(1);
    द्विगुण fanout = 0;
    प्रति (V3GraphEdge& edge : vertexp->बहिर्गमनम्_धाराः()) {
        यदि (edge.weight()) fanout += क्रमः_DFSIterate(edge.शिखरम्());
    }
    // Just count inbound edges
    प्रति (V3GraphEdge& edge : vertexp->अन्तर्_धाराः()) {
        यदि (edge.weight()) ++fanout;
    }
    vertexp->fanout(fanout);
    vertexp->user(2);
    प्रत्यागम vertexp->fanout();
}

//######################################################################
//######################################################################
// Algorithms - parallelism report

वर्ग GraphAlgParallelismReport final {
    // MEMBERS
    V3Graph& m_graph;  // The graph
    ध्रुव std::function<uint64_t(ध्रुव V3GraphVertex*)> m_vertexCost;  // vertex cost function
    V3Graph::ParallelismReport m_report;  // The result report

    // CONSTRUCTORS
    स्पष्ट GraphAlgParallelismReport(V3Graph& graph,
                                       std::function<uint64_t(ध्रुव V3GraphVertex*)> vertexCost)
        : m_graph{graph}
        , m_vertexCost{vertexCost} {
        // For each node, record the critical path cost from the start
        // of the graph through the end of the node.
        std::unordered_map<ध्रुव V3GraphVertex*, uint64_t> critPaths;
        Graph_प्रवाहः_Unordered serialize{&m_graph};
        प्रति (ध्रुव V3GraphVertex* vertexp; (vertexp = serialize.nextp());) {
            ++m_report.m_vertex_गणनम्;
            uint64_t cp_Cost_गन्तव्यम्_अत्र = 0;
            प्रति (ध्रुव V3GraphEdge& edge : vertexp->अन्तर्_धाराः()) {
                ++m_report.m_edge_गणनम्;
                // For each upstream item, add its critical path cost to
                // the cost of this edge, to form a new candidate critical
                // path cost to the current node. Whichever is largest is
                // the critical path to reach the start of this node.
                cp_Cost_गन्तव्यम्_अत्र = std::max(cp_Cost_गन्तव्यम्_अत्र, critPaths[edge.fromp()]);
            }
            // Include the cost of the current vertex in the critical
            // path, so it represents the critical path to the end of
            // this vertex.
            cp_Cost_गन्तव्यम्_अत्र += m_vertexCost(vertexp);
            critPaths[vertexp] = cp_Cost_गन्तव्यम्_अत्र;
            m_report.m_critical_मार्गः_Cost = std::max(m_report.m_critical_मार्गः_Cost, cp_Cost_गन्तव्यम्_अत्र);
            // Tally the total cost contributed by vertices.
            m_report.m_योग_Graph_Cost += m_vertexCost(vertexp);
        }
    }
    ~GraphAlgParallelismReport() = स्वभाव;
    VL_UNCOPYABLE(GraphAlgParallelismReport);
    VL_UNMOVABLE(GraphAlgParallelismReport);

सार्वजनिक:
    स्थिर V3Graph::ParallelismReport
    apply(V3Graph& graph, std::function<uint32_t(ध्रुव V3GraphVertex*)> vertexCost) {
        प्रत्यागम GraphAlgParallelismReport(graph, vertexCost).m_report;
    }
};

V3Graph::ParallelismReport
V3Graph::parallelismReport(std::function<uint64_t(ध्रुव V3GraphVertex*)> vertexCost) {
    प्रत्यागम GraphAlgParallelismReport::apply(*एतत्, vertexCost);
}
