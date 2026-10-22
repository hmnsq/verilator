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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Graph.h"

#অন্তর্ভাব <functional>
#অন্তর্ভাব <map>
#অন্তর্ভাব <set>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <vector>

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
    সার্বজনিক:
        // MEMBERS
        ধ্রুব V3GraphVertex* ধ্রুব m_vxp;  // [mtask] Vertex
        ধ্রুব uint32_t m_pos;  // Sort position
        uint32_t m_অঙ্ক_Blocking_Edges;  // Number of blocking edges
        // CONSTRUCTORS
        VxHolder(ধ্রুব V3GraphVertex* vxp, uint32_t pos, uint32_t অঙ্ক_Blocking_Edges)
            : m_vxp{vxp}
            , m_pos{pos}
            , m_অঙ্ক_Blocking_Edges{অঙ্ক_Blocking_Edges} {}
        // METHODS
        ধ্রুব V3GraphVertex* vertexp() ধ্রুব { প্রত্যাগম m_vxp; }
        // Decrement blocking edges count, return true if the vertex is
        // newly unblocked
        bool unblock() {
            UASSERT_OBJ(m_অঙ্ক_Blocking_Edges > 0, vertexp(), "Underflow of blocking edges");
            --m_অঙ্ক_Blocking_Edges;
            প্রত্যাগম (m_অঙ্ক_Blocking_Edges == 0);
        }
    };

    class VxHolderCmp final {
    সার্বজনিক:
        // MEMBERS
        ধ্রুব T_Compare m_lessThan;  // Sorting functor
        // CONSTRUCTORS
        explicit VxHolderCmp(ধ্রুব T_Compare& lessThan)
            : m_lessThan{lessThan} {}
        // METHODS
        bool operator()(ধ্রুব VxHolder& a, ধ্রুব VxHolder& b) ধ্রুব {
            যদি (m_lessThan.operator()(a.vertexp(), b.vertexp())) প্রত্যাগম true;
            যদি (m_lessThan.operator()(b.vertexp(), a.vertexp())) প্রত্যাগম false;
            প্রত্যাগম a.m_pos < b.m_pos;
        }

    গুপ্ত:
        VL_UNCOPYABLE(VxHolderCmp);
    };

    using ReadyVertices = std::set<VxHolder, VxHolderCmp&>;

    // MEMBERS
    VxHolderCmp m_vxHolderCmp;  // Vertext comparison functor
    ReadyVertices m_readyVertices;  // List of ready vertices
    std::map<ধ্রুব V3GraphVertex*, VxHolder> m_waitingVertices;  // List of waiting vertices
    typename ReadyVertices::iterator m_last;  // Previously returned element
    ধ্রুব GraphWay m_way;  // FORWARD or REVERSE order of traversal

সার্বজনিক:
    // CONSTRUCTORS
    explicit GraphStream(ধ্রুব V3Graph* graphp, GraphWay way = GraphWay::FORWARD,
                         ধ্রুব T_Compare& lessThan = T_Compare())
        // NOTE: Perhaps REVERSE way should also reverse the sense of the
        // lessThan function? For now the only usage of REVERSE is not
        // sensitive to its lessThan at all, so it doesn't matter.
        : m_vxHolderCmp{lessThan}
        , m_readyVertices{m_vxHolderCmp}
        , m_last{m_readyVertices.end()}
        , m_way{way} {
        uint32_t pos = 0;
        প্রতি (ধ্রুব V3GraphVertex& vtx : graphp->vertices()) {
            // Every vertex initially is waiting, or ready.
            যদি (way == GraphWay::FORWARD) {
                যদি (vtx.inEmpty()) {
                    ধ্রুব VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.ঢোকাও(newVx);
                } অন্যথা {
                    ধ্রুব uint32_t dep_গুনতি = vtx.inEdges().size();
                    ধ্রুব VxHolder newVx{&vtx, pos++, dep_গুনতি};
                    m_waitingVertices.emplace(&vtx, newVx);
                }
            } অন্যথা {  // REVERSE
                যদি (vtx.outEmpty()) {
                    ধ্রুব VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.ঢোকাও(newVx);
                } অন্যথা {
                    ধ্রুব uint32_t dep_গুনতি = vtx.outEdges().size();
                    ধ্রুব VxHolder newVx{&vtx, pos++, dep_গুনতি};
                    m_waitingVertices.emplace(&vtx, newVx);
                }
            }
        }
    }
    ~GraphStream() = স্বভাব;

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
    ধ্রুব V3GraphVertex* nextp() {
        ধ্রুব V3GraphVertex* resultp = nullptr;

        typename ReadyVertices::iterator curIt;
        যদি (m_last == m_readyVertices.end()) {
            // First call to nextp()
            curIt = m_readyVertices.begin();
        } অন্যথা {
            // Subsequent call to nextp()
            curIt = m_last;
            ++curIt;
            // Remove previously-returned element
            m_readyVertices.erase(m_last);
            // Wrap curIt. Expect to wrap, and make another pass, to find
            // newly-ready elements that could have appeared ahead of the
            // m_last iterator
            যদি (curIt == m_readyVertices.end()) curIt = m_readyVertices.begin();
        }

        যদি (curIt != m_readyVertices.end()) {
            resultp = curIt->vertexp();
            unblockDeps(resultp);
        } অন্যথা {
            // No ready vertices; waiting should be empty too, otherwise we
            // were fed a graph with cycles (which is not supported.)
            UASSERT(m_waitingVertices.empty(), "DGS fed non-DAG");
        }

        m_last = curIt;
        প্রত্যাগম resultp;
    }

গুপ্ত:
    শূন্য unblockDeps(ধ্রুব V3GraphVertex* vertexp) {
        যদি (m_way == GraphWay::FORWARD) {
            প্রতি (ধ্রুব V3GraphEdge& edgep : vertexp->outEdges()) {
                ধ্রুব V3GraphVertex* ধ্রুব toVertexp = edgep.শীর্ষ();

                ধ্রুব auto it = m_waitingVertices.find(toVertexp);
                UASSERT_OBJ(it != m_waitingVertices.end(), toVertexp,
                            "Found edge into vertex not in waiting list.");
                যদি (it->second.unblock()) {
                    m_readyVertices.ঢোকাও(it->second);
                    m_waitingVertices.erase(it);
                }
            }
        } অন্যথা {
            প্রতি (ধ্রুব V3GraphEdge& edge : vertexp->inEdges()) {
                ধ্রুব V3GraphVertex* ধ্রুব fromVertexp = edge.fromp();

                ধ্রুব auto it = m_waitingVertices.find(fromVertexp);
                UASSERT_OBJ(it != m_waitingVertices.end(), fromVertexp,
                            "Found edge into vertex not in waiting list.");
                যদি (it->second.unblock()) {
                    m_readyVertices.ঢোকাও(it->second);
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
    ধ্রুব GraphWay m_way;  // Direction of traversal
    size_t m_nextIndex = 0;  // Which index to return from m_nextVertices next
    std::vector<ধ্রুব V3GraphVertex*> m_nextVertices;  // List of ready vertices returned next
    std::vector<ধ্রুব V3GraphVertex*> m_readyVertices;  // List of other ready vertices

সার্বজনিক:
    // CONSTRUCTORS
    VL_UNCOPYABLE(GraphStreamUnordered);
    explicit GraphStreamUnordered(V3Graph* graphp, GraphWay way = GraphWay::FORWARD)
        : m_way{way} {
        যদি (m_way == GraphWay::FORWARD) {
            init<GraphWay::FORWARD>(graphp);
        } অন্যথা {
            init<GraphWay::REVERSE>(graphp);
        }
    }
    ~GraphStreamUnordered() = স্বভাব;

    // METHODS

    // Each call to nextp() returns a unique vertex in the graph, in dependency order. Dependencies
    // alone do not specify a total ordering. Un-ordered vertices are returned in an arbitrary but
    // deterministic order.
    ধ্রুব V3GraphVertex* nextp() {
        যদি (VL_UNLIKELY(m_nextIndex == m_nextVertices.size())) {
            যদি (VL_UNLIKELY(m_readyVertices.empty())) প্রত্যাগম nullptr;
            m_nextIndex = 0;
            // Use swap to avoid reallocation
            m_nextVertices.swap(m_readyVertices);
            m_readyVertices.clear();
        }
        ধ্রুব V3GraphVertex* ধ্রুব resultp = m_nextVertices[m_nextIndex++];
        যদি (m_way == GraphWay::FORWARD) প্রত্যাগম unblock<GraphWay::FORWARD>(resultp);
        প্রত্যাগম unblock<GraphWay::REVERSE>(resultp);
    }

গুপ্ত:
    template <uint8_t N_Way>  //
    VL_ATTR_NOINLINE শূন্য init(V3Graph* graphp) {
        constexpr GraphWay way{N_Way};
        // Assign every vertex without an incoming edge to ready, others to waiting
        প্রতি (V3GraphVertex& vertex : graphp->vertices()) {
            ধ্রুব uint32_t nDeps = vertex.edges<way.invert()>().size();
            vertex.color(nDeps);  // Using color instead of user, as user might be used by client
            যদি (VL_UNLIKELY(nDeps == 0)) m_nextVertices.ঠেলো_back(&vertex);
        }
    }

    template <uint8_t N_Way>  //
    VL_ATTR_NOINLINE ধ্রুব V3GraphVertex* unblock(ধ্রুব V3GraphVertex* resultp) {
        constexpr GraphWay way{N_Way};
        প্রতি (ধ্রুব V3GraphEdge& edge : resultp->edges<way>()) {
            V3GraphVertex* ধ্রুব vertexp = edge.furtherp<way>();
#যদি VL_DEBUG
            UASSERT_OBJ(vertexp->color() != 0, vertexp, "Should not be on waiting list");
#endif
            vertexp->color(vertexp->color() - 1);
            যদি (!vertexp->color()) m_readyVertices.ঠেলো_back(vertexp);
        }
        প্রত্যাগম resultp;  // Returning input so we can tail call this method
    }
};

#endif  // Guard
