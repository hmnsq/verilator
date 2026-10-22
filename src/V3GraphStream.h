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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Graph.h"

#ضمن <functional>
#ضمن <map>
#ضمن <set>
#ضمن <unordered_map>
#ضمن <vector>

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
    عام:
        // MEMBERS
        ثابت V3GraphVertex* ثابت m_vxp;  // [mtask] Vertex
        ثابت uint32_t m_pos;  // Sort position
        uint32_t m_رەقەم_Blocking_Edges;  // Number of blocking edges
        // CONSTRUCTORS
        VxHolder(ثابت V3GraphVertex* vxp, uint32_t pos, uint32_t رەقەم_Blocking_Edges)
            : m_vxp{vxp}
            , m_pos{pos}
            , m_رەقەم_Blocking_Edges{رەقەم_Blocking_Edges} {}
        // METHODS
        ثابت V3GraphVertex* vertexp() ثابت { أرجع m_vxp; }
        // Decrement blocking edges count, return true if the vertex is
        // newly unblocked
        bool unblock() {
            UASSERT_OBJ(m_رەقەم_Blocking_Edges > 0, vertexp(), "Underflow of blocking edges");
            --m_رەقەم_Blocking_Edges;
            أرجع (m_رەقەم_Blocking_Edges == 0);
        }
    };

    class VxHolderCmp final {
    عام:
        // MEMBERS
        ثابت T_Compare m_lessThan;  // Sorting functor
        // CONSTRUCTORS
        explicit VxHolderCmp(ثابت T_Compare& lessThan)
            : m_lessThan{lessThan} {}
        // METHODS
        bool operator()(ثابت VxHolder& a, ثابت VxHolder& b) ثابت {
            إذا (m_lessThan.operator()(a.vertexp(), b.vertexp())) أرجع true;
            إذا (m_lessThan.operator()(b.vertexp(), a.vertexp())) أرجع false;
            أرجع a.m_pos < b.m_pos;
        }

    خاص:
        VL_UNCOPYABLE(VxHolderCmp);
    };

    using ReadyVertices = std::set<VxHolder, VxHolderCmp&>;

    // MEMBERS
    VxHolderCmp m_vxHolderCmp;  // Vertext comparison functor
    ReadyVertices m_readyVertices;  // List of ready vertices
    std::map<ثابت V3GraphVertex*, VxHolder> m_waitingVertices;  // List of waiting vertices
    typename ReadyVertices::iterator m_last;  // Previously returned element
    ثابت GraphWay m_way;  // FORWARD or REVERSE order of traversal

عام:
    // CONSTRUCTORS
    explicit GraphStream(ثابت V3Graph* graphp, GraphWay way = GraphWay::FORWARD,
                         ثابت T_Compare& lessThan = T_Compare())
        // NOTE: Perhaps REVERSE way should also reverse the sense of the
        // lessThan function? For now the only usage of REVERSE is not
        // sensitive to its lessThan at all, so it doesn't matter.
        : m_vxHolderCmp{lessThan}
        , m_readyVertices{m_vxHolderCmp}
        , m_last{m_readyVertices.end()}
        , m_way{way} {
        uint32_t pos = 0;
        لكل (ثابت V3GraphVertex& vtx : graphp->vertices()) {
            // Every vertex initially is waiting, or ready.
            إذا (way == GraphWay::FORWARD) {
                إذا (vtx.inEmpty()) {
                    ثابت VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.قىستۇرۇش(newVx);
                } وإلا {
                    ثابت uint32_t dep_سانى = vtx.inEdges().size();
                    ثابت VxHolder newVx{&vtx, pos++, dep_سانى};
                    m_waitingVertices.emplace(&vtx, newVx);
                }
            } وإلا {  // REVERSE
                إذا (vtx.outEmpty()) {
                    ثابت VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.قىستۇرۇش(newVx);
                } وإلا {
                    ثابت uint32_t dep_سانى = vtx.outEdges().size();
                    ثابت VxHolder newVx{&vtx, pos++, dep_سانى};
                    m_waitingVertices.emplace(&vtx, newVx);
                }
            }
        }
    }
    ~GraphStream() = افتراضي;

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
    ثابت V3GraphVertex* nextp() {
        ثابت V3GraphVertex* resultp = nullptr;

        typename ReadyVertices::iterator curIt;
        إذا (m_last == m_readyVertices.end()) {
            // First call to nextp()
            curIt = m_readyVertices.begin();
        } وإلا {
            // Subsequent call to nextp()
            curIt = m_last;
            ++curIt;
            // Remove previously-returned element
            m_readyVertices.erase(m_last);
            // Wrap curIt. Expect to wrap, and make another pass, to find
            // newly-ready elements that could have appeared ahead of the
            // m_last iterator
            إذا (curIt == m_readyVertices.end()) curIt = m_readyVertices.begin();
        }

        إذا (curIt != m_readyVertices.end()) {
            resultp = curIt->vertexp();
            unblockDeps(resultp);
        } وإلا {
            // No ready vertices; waiting should be empty too, otherwise we
            // were fed a graph with cycles (which is not supported.)
            UASSERT(m_waitingVertices.empty(), "DGS fed non-DAG");
        }

        m_last = curIt;
        أرجع resultp;
    }

خاص:
    فراغ unblockDeps(ثابت V3GraphVertex* vertexp) {
        إذا (m_way == GraphWay::FORWARD) {
            لكل (ثابت V3GraphEdge& edgep : vertexp->outEdges()) {
                ثابت V3GraphVertex* ثابت toVertexp = edgep.چوققا();

                ثابت auto it = m_waitingVertices.find(toVertexp);
                UASSERT_OBJ(it != m_waitingVertices.end(), toVertexp,
                            "Found edge into vertex not in waiting list.");
                إذا (it->second.unblock()) {
                    m_readyVertices.قىستۇرۇش(it->second);
                    m_waitingVertices.erase(it);
                }
            }
        } وإلا {
            لكل (ثابت V3GraphEdge& edge : vertexp->inEdges()) {
                ثابت V3GraphVertex* ثابت fromVertexp = edge.fromp();

                ثابت auto it = m_waitingVertices.find(fromVertexp);
                UASSERT_OBJ(it != m_waitingVertices.end(), fromVertexp,
                            "Found edge into vertex not in waiting list.");
                إذا (it->second.unblock()) {
                    m_readyVertices.قىستۇرۇش(it->second);
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
    ثابت GraphWay m_way;  // Direction of traversal
    size_t m_nextIndex = 0;  // Which index to return from m_nextVertices next
    std::vector<ثابت V3GraphVertex*> m_nextVertices;  // List of ready vertices returned next
    std::vector<ثابت V3GraphVertex*> m_readyVertices;  // List of other ready vertices

عام:
    // CONSTRUCTORS
    VL_UNCOPYABLE(GraphStreamUnordered);
    explicit GraphStreamUnordered(V3Graph* graphp, GraphWay way = GraphWay::FORWARD)
        : m_way{way} {
        إذا (m_way == GraphWay::FORWARD) {
            init<GraphWay::FORWARD>(graphp);
        } وإلا {
            init<GraphWay::REVERSE>(graphp);
        }
    }
    ~GraphStreamUnordered() = افتراضي;

    // METHODS

    // Each call to nextp() returns a unique vertex in the graph, in dependency order. Dependencies
    // alone do not specify a total ordering. Un-ordered vertices are returned in an arbitrary but
    // deterministic order.
    ثابت V3GraphVertex* nextp() {
        إذا (VL_UNLIKELY(m_nextIndex == m_nextVertices.size())) {
            إذا (VL_UNLIKELY(m_readyVertices.empty())) أرجع nullptr;
            m_nextIndex = 0;
            // Use swap to avoid reallocation
            m_nextVertices.swap(m_readyVertices);
            m_readyVertices.clear();
        }
        ثابت V3GraphVertex* ثابت resultp = m_nextVertices[m_nextIndex++];
        إذا (m_way == GraphWay::FORWARD) أرجع unblock<GraphWay::FORWARD>(resultp);
        أرجع unblock<GraphWay::REVERSE>(resultp);
    }

خاص:
    template <uint8_t N_Way>  //
    VL_ATTR_NOINLINE فراغ init(V3Graph* graphp) {
        constexpr GraphWay way{N_Way};
        // Assign every vertex without an incoming edge to ready, others to waiting
        لكل (V3GraphVertex& vertex : graphp->vertices()) {
            ثابت uint32_t nDeps = vertex.edges<way.invert()>().size();
            vertex.color(nDeps);  // Using color instead of user, as user might be used by client
            إذا (VL_UNLIKELY(nDeps == 0)) m_nextVertices.ئىتتىرىش_back(&vertex);
        }
    }

    template <uint8_t N_Way>  //
    VL_ATTR_NOINLINE ثابت V3GraphVertex* unblock(ثابت V3GraphVertex* resultp) {
        constexpr GraphWay way{N_Way};
        لكل (ثابت V3GraphEdge& edge : resultp->edges<way>()) {
            V3GraphVertex* ثابت vertexp = edge.furtherp<way>();
#إذا VL_DEBUG
            UASSERT_OBJ(vertexp->color() != 0, vertexp, "Should not be on waiting list");
#endif
            vertexp->color(vertexp->color() - 1);
            إذا (!vertexp->color()) m_readyVertices.ئىتتىرىش_back(vertexp);
        }
        أرجع resultp;  // Returning input so we can tail call this method
    }
};

#endif  // Guard
