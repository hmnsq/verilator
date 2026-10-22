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

#ifndef VERILATOR_V3GRAPHSTREAM_H_
#define VERILATOR_V3GRAPHSTREAM_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Graph.h"

#अन्तर्भाव <functional>
#अन्तर्भाव <map>
#अन्तर्भाव <set>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

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

template <typename T_Compare>
class GraphStream final {
    // TYPES
    class VxHolder final {
    सार्वजनिक:
        // MEMBERS
        ध्रुव V3GraphVertex* ध्रुव m_vxp;  // [mtask] Vertex
        ध्रुव uint32_t m_pos;  // Sort position
        uint32_t m_अंक_Blocking_Edges;  // Number of blocking edges
        // CONSTRUCTORS
        VxHolder(ध्रुव V3GraphVertex* vxp, uint32_t pos, uint32_t अंक_Blocking_Edges)
            : m_vxp{vxp}
            , m_pos{pos}
            , m_अंक_Blocking_Edges{अंक_Blocking_Edges} {}
        // METHODS
        ध्रुव V3GraphVertex* vertexp() ध्रुव { प्रत्यागम m_vxp; }
        // Decrement blocking edges count, return true if the vertex is
        // newly unblocked
        bool unblock() {
            UASSERT_OBJ(m_अंक_Blocking_Edges > 0, vertexp(), "Underflow of blocking edges");
            --m_अंक_Blocking_Edges;
            प्रत्यागम (m_अंक_Blocking_Edges == 0);
        }
    };

    class VxHolderCmp final {
    सार्वजनिक:
        // MEMBERS
        ध्रुव T_Compare m_lessThan;  // Sorting functor
        // CONSTRUCTORS
        explicit VxHolderCmp(ध्रुव T_Compare& lessThan)
            : m_lessThan{lessThan} {}
        // METHODS
        bool operator()(ध्रुव VxHolder& a, ध्रुव VxHolder& b) ध्रुव {
            यदि (m_lessThan.operator()(a.vertexp(), b.vertexp())) प्रत्यागम true;
            यदि (m_lessThan.operator()(b.vertexp(), a.vertexp())) प्रत्यागम false;
            प्रत्यागम a.m_pos < b.m_pos;
        }

    गुप्त:
        VL_UNCOPYABLE(VxHolderCmp);
    };

    using ReadyVertices = std::set<VxHolder, VxHolderCmp&>;

    // MEMBERS
    VxHolderCmp m_vxHolderCmp;  // Vertext comparison functor
    ReadyVertices m_readyVertices;  // List of ready vertices
    std::map<ध्रुव V3GraphVertex*, VxHolder> m_waitingVertices;  // List of waiting vertices
    typename ReadyVertices::iterator m_last;  // Previously returned element
    ध्रुव GraphWay m_way;  // FORWARD or REVERSE order of traversal

सार्वजनिक:
    // CONSTRUCTORS
    explicit GraphStream(ध्रुव V3Graph* graphp, GraphWay way = GraphWay::FORWARD,
                         ध्रुव T_Compare& lessThan = T_Compare())
        // NOTE: Perhaps REVERSE way should also reverse the sense of the
        // lessThan function? For now the only usage of REVERSE is not
        // sensitive to its lessThan at all, so it doesn't matter.
        : m_vxHolderCmp{lessThan}
        , m_readyVertices{m_vxHolderCmp}
        , m_last{m_readyVertices.end()}
        , m_way{way} {
        uint32_t pos = 0;
        प्रति (ध्रुव V3GraphVertex& vtx : graphp->vertices()) {
            // Every vertex initially is waiting, or ready.
            यदि (way == GraphWay::FORWARD) {
                यदि (vtx.inEmpty()) {
                    ध्रुव VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.घाला(newVx);
                } अन्यथा {
                    ध्रुव uint32_t dep_मोजणी = vtx.inEdges().size();
                    ध्रुव VxHolder newVx{&vtx, pos++, dep_मोजणी};
                    m_waitingVertices.emplace(&vtx, newVx);
                }
            } अन्यथा {  // REVERSE
                यदि (vtx.outEmpty()) {
                    ध्रुव VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.घाला(newVx);
                } अन्यथा {
                    ध्रुव uint32_t dep_मोजणी = vtx.outEdges().size();
                    ध्रुव VxHolder newVx{&vtx, pos++, dep_मोजणी};
                    m_waitingVertices.emplace(&vtx, newVx);
                }
            }
        }
    }
    ~GraphStream() = स्वभाव;

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
    ध्रुव V3GraphVertex* nextp() {
        ध्रुव V3GraphVertex* resultp = nullptr;

        typename ReadyVertices::iterator curIt;
        यदि (m_last == m_readyVertices.end()) {
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
            यदि (curIt == m_readyVertices.end()) curIt = m_readyVertices.begin();
        }

        यदि (curIt != m_readyVertices.end()) {
            resultp = curIt->vertexp();
            unblockDeps(resultp);
        } अन्यथा {
            // No ready vertices; waiting should be empty too, otherwise we
            // were fed a graph with cycles (which is not supported.)
            UASSERT(m_waitingVertices.empty(), "DGS fed non-DAG");
        }

        m_last = curIt;
        प्रत्यागम resultp;
    }

गुप्त:
    शून्य unblockDeps(ध्रुव V3GraphVertex* vertexp) {
        यदि (m_way == GraphWay::FORWARD) {
            प्रति (ध्रुव V3GraphEdge& edgep : vertexp->outEdges()) {
                ध्रुव V3GraphVertex* ध्रुव toVertexp = edgep.शीर्ष();

                ध्रुव auto it = m_waitingVertices.find(toVertexp);
                UASSERT_OBJ(it != m_waitingVertices.end(), toVertexp,
                            "Found edge into vertex not in waiting list.");
                यदि (it->second.unblock()) {
                    m_readyVertices.घाला(it->second);
                    m_waitingVertices.erase(it);
                }
            }
        } अन्यथा {
            प्रति (ध्रुव V3GraphEdge& edge : vertexp->inEdges()) {
                ध्रुव V3GraphVertex* ध्रुव fromVertexp = edge.fromp();

                ध्रुव auto it = m_waitingVertices.find(fromVertexp);
                UASSERT_OBJ(it != m_waitingVertices.end(), fromVertexp,
                            "Found edge into vertex not in waiting list.");
                यदि (it->second.unblock()) {
                    m_readyVertices.घाला(it->second);
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
    ध्रुव GraphWay m_way;  // Direction of traversal
    size_t m_nextIndex = 0;  // Which index to return from m_nextVertices next
    std::vector<ध्रुव V3GraphVertex*> m_nextVertices;  // List of ready vertices returned next
    std::vector<ध्रुव V3GraphVertex*> m_readyVertices;  // List of other ready vertices

सार्वजनिक:
    // CONSTRUCTORS
    VL_UNCOPYABLE(GraphStreamUnordered);
    explicit GraphStreamUnordered(V3Graph* graphp, GraphWay way = GraphWay::FORWARD)
        : m_way{way} {
        यदि (m_way == GraphWay::FORWARD) {
            init<GraphWay::FORWARD>(graphp);
        } अन्यथा {
            init<GraphWay::REVERSE>(graphp);
        }
    }
    ~GraphStreamUnordered() = स्वभाव;

    // METHODS

    // Each call to nextp() returns a unique vertex in the graph, in dependency order. Dependencies
    // alone do not specify a total ordering. Un-ordered vertices are returned in an arbitrary but
    // deterministic order.
    ध्रुव V3GraphVertex* nextp() {
        यदि (VL_UNLIKELY(m_nextIndex == m_nextVertices.size())) {
            यदि (VL_UNLIKELY(m_readyVertices.empty())) प्रत्यागम nullptr;
            m_nextIndex = 0;
            // Use swap to avoid reallocation
            m_nextVertices.swap(m_readyVertices);
            m_readyVertices.clear();
        }
        ध्रुव V3GraphVertex* ध्रुव resultp = m_nextVertices[m_nextIndex++];
        यदि (m_way == GraphWay::FORWARD) प्रत्यागम unblock<GraphWay::FORWARD>(resultp);
        प्रत्यागम unblock<GraphWay::REVERSE>(resultp);
    }

गुप्त:
    template <uint8_t N_Way>  //
    VL_ATTR_NOINLINE शून्य init(V3Graph* graphp) {
        constexpr GraphWay way{N_Way};
        // Assign every vertex without an incoming edge to ready, others to waiting
        प्रति (V3GraphVertex& vertex : graphp->vertices()) {
            ध्रुव uint32_t nDeps = vertex.edges<way.invert()>().size();
            vertex.color(nDeps);  // Using color instead of user, as user might be used by client
            यदि (VL_UNLIKELY(nDeps == 0)) m_nextVertices.ढकला_back(&vertex);
        }
    }

    template <uint8_t N_Way>  //
    VL_ATTR_NOINLINE ध्रुव V3GraphVertex* unblock(ध्रुव V3GraphVertex* resultp) {
        constexpr GraphWay way{N_Way};
        प्रति (ध्रुव V3GraphEdge& edge : resultp->edges<way>()) {
            V3GraphVertex* ध्रुव vertexp = edge.furtherp<way>();
#यदि VL_DEBUG
            UASSERT_OBJ(vertexp->color() != 0, vertexp, "Should not be on waiting list");
#endif
            vertexp->color(vertexp->color() - 1);
            यदि (!vertexp->color()) m_readyVertices.ढकला_back(vertexp);
        }
        प्रत्यागम resultp;  // Returning input so we can tail call this method
    }
};

#endif  // Guard
