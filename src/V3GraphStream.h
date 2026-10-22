// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Dependency graph iterator. Iterates over nodes
//                         in any DAG, following dependency order.
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

#अगर_अघोषित VERILATOR_V3GRAPHSTREAM_H_
#घोषणा VERILATOR_V3GRAPHSTREAM_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Graph.h"

#समावेश <functional>
#समावेश <map>
#समावेश <set>
#समावेश <unordered_map>
#समावेश <vector>

//######################################################################
// GraphStream
//
// Template 'T_Compare' is a tie-breaker for ordering nodes that the DAG
// itself does not order. It must provide an operator() that does a logical
// less-than on two V3GraphVertex*'s, with the same signature as
// std::less<const V3GraphVertex*>::operator().  This does not default to
// std::less<const V3GraphVertex*> because that is nondeterministic, and so
// not generally safe. If you want a raw pointer compare, see
// GraphStreamUnordered below.

ढाँचा <typename T_Compare>
class GraphStream final {
    // TYPES
    class VxHolder final {
    खुला:
        // MEMBERS
        स्थिर V3GraphVertex* स्थिर m_vxp;  // [mtask] Vertex
        स्थिर uint32_t m_pos;  // Sort position
        uint32_t m_अंक_Blocking_Edges;  // Number of blocking edges
        // CONSTRUCTORS
        VxHolder(स्थिर V3GraphVertex* vxp, uint32_t pos, uint32_t अंक_Blocking_Edges)
            : m_vxp{vxp}
            , m_pos{pos}
            , m_अंक_Blocking_Edges{अंक_Blocking_Edges} {}
        // METHODS
        स्थिर V3GraphVertex* vertexp() स्थिर { वापस m_vxp; }
        // Decrement blocking edges count, return true if the vertex is
        // newly unblocked
        bool unblock() {
            UASSERT_OBJ(m_अंक_Blocking_Edges > 0, vertexp(), "Underflow of blocking edges");
            --m_अंक_Blocking_Edges;
            वापस (m_अंक_Blocking_Edges == 0);
        }
    };

    class VxHolderCmp final {
    खुला:
        // MEMBERS
        स्थिर T_Compare m_lessThan;  // Sorting functor
        // CONSTRUCTORS
        explicit VxHolderCmp(स्थिर T_Compare& lessThan)
            : m_lessThan{lessThan} {}
        // METHODS
        bool चालक()(स्थिर VxHolder& a, स्थिर VxHolder& b) स्थिर {
            अगर (m_lessThan.चालक()(a.vertexp(), b.vertexp())) वापस true;
            अगर (m_lessThan.चालक()(b.vertexp(), a.vertexp())) वापस false;
            वापस a.m_pos < b.m_pos;
        }

    निजी:
        VL_UNCOPYABLE(VxHolderCmp);
    };

    using ReadyVertices = std::set<VxHolder, VxHolderCmp&>;

    // MEMBERS
    VxHolderCmp m_vxHolderCmp;  // Vertext comparison functor
    ReadyVertices m_readyVertices;  // List of ready vertices
    std::map<स्थिर V3GraphVertex*, VxHolder> m_waitingVertices;  // List of waiting vertices
    typename ReadyVertices::iterator m_last;  // Previously returned element
    स्थिर GraphWay m_way;  // FORWARD or REVERSE order of traversal

खुला:
    // CONSTRUCTORS
    explicit GraphStream(स्थिर V3Graph* graphp, GraphWay way = GraphWay::FORWARD,
                         स्थिर T_Compare& lessThan = T_Compare())
        // NOTE: Perhaps REVERSE way should also reverse the sense of the
        // lessThan function? For now the only usage of REVERSE is not
        // sensitive to its lessThan at all, so it doesn't matter.
        : m_vxHolderCmp{lessThan}
        , m_readyVertices{m_vxHolderCmp}
        , m_last{m_readyVertices.end()}
        , m_way{way} {
        uint32_t pos = 0;
        क्रम (स्थिर V3GraphVertex& vtx : graphp->vertices()) {
            // Every vertex initially is waiting, or ready.
            अगर (way == GraphWay::FORWARD) {
                अगर (vtx.inEmpty()) {
                    स्थिर VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.जोड़ो(newVx);
                } अन्यथा {
                    स्थिर uint32_t dep_गिनती = vtx.inEdges().आकार();
                    स्थिर VxHolder newVx{&vtx, pos++, dep_गिनती};
                    m_waitingVertices.emplace(&vtx, newVx);
                }
            } अन्यथा {  // REVERSE
                अगर (vtx.outEmpty()) {
                    स्थिर VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.जोड़ो(newVx);
                } अन्यथा {
                    स्थिर uint32_t dep_गिनती = vtx.outEdges().आकार();
                    स्थिर VxHolder newVx{&vtx, pos++, dep_गिनती};
                    m_waitingVertices.emplace(&vtx, newVx);
                }
            }
        }
    }
    ~GraphStream() = शेष;

    // METHODS

    // Each call to nextp() returns a unique vertex in the graph, in
    // dependency order.
    //
    // Dependencies alone don't fully specify the order. Usually a graph
    // has many "ready" vertices, any of which might return next.
    //
    // To decide among the "ready" vertices, GraphStream keeps an ordered
    // list of ready vertices, sorted first by lessThan and second by
    // original graph order.
    //
    // You might expect that nextp() would return the first item from this
    // sorted list -- but that's not what it does!  What nextp() actually
    // does is to return the next item in the list, following the position
    // where the previously-returned item would have been.  This maximizes
    // locality: given an appropriate lessThan, nextp() will stay on a
    // given domain (or domscope, or mtask, or whatever) for as long as
    // possible before an unmet dependency forces us to switch to another
    // one.
    //
    // Within a group of vertices that lessThan considers equivalent,
    // nextp() returns them in the original graph order (presumably also
    // good locality.) V3Order.cpp relies on this to order the logic
    // vertices within a given mtask without jumping over domains too much.
    स्थिर V3GraphVertex* nextp() {
        स्थिर V3GraphVertex* resultp = nullptr;

        typename ReadyVertices::iterator curIt;
        अगर (m_last == m_readyVertices.end()) {
            // First call to nextp()
            curIt = m_readyVertices.begin();
        } अन्यथा {
            // Subsequent call to nextp()
            curIt = m_last;
            ++curIt;
            // Remove previously-returned element
            m_readyVertices.erase(m_last);
            // Wrap curIt. Expect to wrap, and make another pass, to find
            // newly-ready elements that could have appeared ahead of the
            // m_last iterator
            अगर (curIt == m_readyVertices.end()) curIt = m_readyVertices.begin();
        }

        अगर (curIt != m_readyVertices.end()) {
            resultp = curIt->vertexp();
            unblockDeps(resultp);
        } अन्यथा {
            // No ready vertices; waiting should be empty too, otherwise we
            // were fed a graph with cycles (which is not supported.)
            UASSERT(m_waitingVertices.empty(), "DGS fed non-DAG");
        }

        m_last = curIt;
        वापस resultp;
    }

निजी:
    व्योम unblockDeps(स्थिर V3GraphVertex* vertexp) {
        अगर (m_way == GraphWay::FORWARD) {
            क्रम (स्थिर V3GraphEdge& edgep : vertexp->outEdges()) {
                स्थिर V3GraphVertex* स्थिर toVertexp = edgep.शीर्ष();

                स्थिर स्वतः it = m_waitingVertices.find(toVertexp);
                UASSERT_OBJ(it != m_waitingVertices.end(), toVertexp,
                            "Found edge into vertex not in waiting list.");
                अगर (it->second.unblock()) {
                    m_readyVertices.जोड़ो(it->second);
                    m_waitingVertices.erase(it);
                }
            }
        } अन्यथा {
            क्रम (स्थिर V3GraphEdge& edge : vertexp->inEdges()) {
                स्थिर V3GraphVertex* स्थिर fromVertexp = edge.fromp();

                स्थिर स्वतः it = m_waitingVertices.find(fromVertexp);
                UASSERT_OBJ(it != m_waitingVertices.end(), fromVertexp,
                            "Found edge into vertex not in waiting list.");
                अगर (it->second.unblock()) {
                    m_readyVertices.जोड़ो(it->second);
                    m_waitingVertices.erase(it);
                }
            }
        }
    }

    VL_UNCOPYABLE(GraphStream);
};

//=================================================================================================
// GraphStreamUnordered is similar to GraphStream, but iterates un-ordered vertices (those that are
// not ordered by dependencies) in an arbitrary order. Iteration order is still deterministic.

class GraphStreamUnordered final {
    // MEMBERS
    स्थिर GraphWay m_way;  // Direction of traversal
    माप_प्रकार m_अग्र_सूचकांक = 0;  // Which index to return from m_nextVertices next
    std::vector<स्थिर V3GraphVertex*> m_अग्र_Vertices;  // List of ready vertices returned next
    std::vector<स्थिर V3GraphVertex*> m_readyVertices;  // List of other ready vertices

खुला:
    // CONSTRUCTORS
    VL_UNCOPYABLE(GraphStreamUnordered);
    explicit GraphStreamUnordered(V3Graph* graphp, GraphWay way = GraphWay::FORWARD)
        : m_way{way} {
        अगर (m_way == GraphWay::FORWARD) {
            init<GraphWay::FORWARD>(graphp);
        } अन्यथा {
            init<GraphWay::REVERSE>(graphp);
        }
    }
    ~GraphStreamUnordered() = शेष;

    // METHODS

    // Each call to nextp() returns a unique vertex in the graph, in dependency order. Dependencies
    // alone do not specify a total ordering. Un-ordered vertices are returned in an arbitrary but
    // deterministic order.
    स्थिर V3GraphVertex* nextp() {
        अगर (VL_UNLIKELY(m_अग्र_सूचकांक == m_अग्र_Vertices.आकार())) {
            अगर (VL_UNLIKELY(m_readyVertices.empty())) वापस nullptr;
            m_अग्र_सूचकांक = 0;
            // Use swap to avoid reallocation
            m_अग्र_Vertices.swap(m_readyVertices);
            m_readyVertices.clear();
        }
        स्थिर V3GraphVertex* स्थिर resultp = m_अग्र_Vertices[m_अग्र_सूचकांक++];
        अगर (m_way == GraphWay::FORWARD) वापस unblock<GraphWay::FORWARD>(resultp);
        वापस unblock<GraphWay::REVERSE>(resultp);
    }

निजी:
    ढाँचा <uint8_t N_Way>  //
    VL_ATTR_NOINLINE व्योम init(V3Graph* graphp) {
        constexpr GraphWay way{N_Way};
        // Assign every vertex without an incoming edge to ready, others to waiting
        क्रम (V3GraphVertex& vertex : graphp->vertices()) {
            स्थिर uint32_t nDeps = vertex.edges<way.invert()>().आकार();
            vertex.color(nDeps);  // Using color instead of user, as user might be used by client
            अगर (VL_UNLIKELY(nDeps == 0)) m_अग्र_Vertices.धकेलो_back(&vertex);
        }
    }

    ढाँचा <uint8_t N_Way>  //
    VL_ATTR_NOINLINE स्थिर V3GraphVertex* unblock(स्थिर V3GraphVertex* resultp) {
        constexpr GraphWay way{N_Way};
        क्रम (स्थिर V3GraphEdge& edge : resultp->edges<way>()) {
            V3GraphVertex* स्थिर vertexp = edge.furtherp<way>();
#अगर VL_DEBUG
            UASSERT_OBJ(vertexp->color() != 0, vertexp, "Should not be on waiting list");
#पूर्ण_अगर
            vertexp->color(vertexp->color() - 1);
            अगर (!vertexp->color()) m_readyVertices.धकेलो_back(vertexp);
        }
        वापस resultp;  // Returning input so we can tail call this method
    }
};

#पूर्ण_अगर  // Guard
