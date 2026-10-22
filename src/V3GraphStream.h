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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Graph.h"

#ಅನ್ತರ್ಭಾವ <functional>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

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
    ಸಾರ್ವಜನಿಕ:
        // MEMBERS
        ಧ್ರುವ V3GraphVertex* ಧ್ರುವ m_vxp;  // [mtask] Vertex
        ಧ್ರುವ uint32_t m_pos;  // Sort position
        uint32_t m_ಅಂಕೆ_Blocking_Edges;  // Number of blocking edges
        // CONSTRUCTORS
        VxHolder(ಧ್ರುವ V3GraphVertex* vxp, uint32_t pos, uint32_t ಅಂಕೆ_Blocking_Edges)
            : m_vxp{vxp}
            , m_pos{pos}
            , m_ಅಂಕೆ_Blocking_Edges{ಅಂಕೆ_Blocking_Edges} {}
        // METHODS
        ಧ್ರುವ V3GraphVertex* vertexp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_vxp; }
        // Decrement blocking edges count, return true if the vertex is
        // newly unblocked
        bool unblock() {
            UASSERT_OBJ(m_ಅಂಕೆ_Blocking_Edges > 0, vertexp(), "Underflow of blocking edges");
            --m_ಅಂಕೆ_Blocking_Edges;
            ಪ್ರತ್ಯಾಗಮ (m_ಅಂಕೆ_Blocking_Edges == 0);
        }
    };

    class VxHolderCmp final {
    ಸಾರ್ವಜನಿಕ:
        // MEMBERS
        ಧ್ರುವ T_Compare m_lessThan;  // Sorting functor
        // CONSTRUCTORS
        explicit VxHolderCmp(ಧ್ರುವ T_Compare& lessThan)
            : m_lessThan{lessThan} {}
        // METHODS
        bool operator()(ಧ್ರುವ VxHolder& a, ಧ್ರುವ VxHolder& b) ಧ್ರುವ {
            ಯದಿ (m_lessThan.operator()(a.vertexp(), b.vertexp())) ಪ್ರತ್ಯಾಗಮ true;
            ಯದಿ (m_lessThan.operator()(b.vertexp(), a.vertexp())) ಪ್ರತ್ಯಾಗಮ false;
            ಪ್ರತ್ಯಾಗಮ a.m_pos < b.m_pos;
        }

    ಗುಪ್ತ:
        VL_UNCOPYABLE(VxHolderCmp);
    };

    using ReadyVertices = std::set<VxHolder, VxHolderCmp&>;

    // MEMBERS
    VxHolderCmp m_vxHolderCmp;  // Vertext comparison functor
    ReadyVertices m_readyVertices;  // List of ready vertices
    std::map<ಧ್ರುವ V3GraphVertex*, VxHolder> m_waitingVertices;  // List of waiting vertices
    typename ReadyVertices::iterator m_last;  // Previously returned element
    ಧ್ರುವ GraphWay m_way;  // FORWARD or REVERSE order of traversal

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit GraphStream(ಧ್ರುವ V3Graph* graphp, GraphWay way = GraphWay::FORWARD,
                         ಧ್ರುವ T_Compare& lessThan = T_Compare())
        // NOTE: Perhaps REVERSE way should also reverse the sense of the
        // lessThan function? For now the only usage of REVERSE is not
        // sensitive to its lessThan at all, so it doesn't matter.
        : m_vxHolderCmp{lessThan}
        , m_readyVertices{m_vxHolderCmp}
        , m_last{m_readyVertices.end()}
        , m_way{way} {
        uint32_t pos = 0;
        ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : graphp->vertices()) {
            // Every vertex initially is waiting, or ready.
            ಯದಿ (way == GraphWay::FORWARD) {
                ಯದಿ (vtx.inEmpty()) {
                    ಧ್ರುವ VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.ಸೇರಿಸು(newVx);
                } ಅನ್ಯಥಾ {
                    ಧ್ರುವ uint32_t dep_ಎಣಿಕೆ = vtx.inEdges().size();
                    ಧ್ರುವ VxHolder newVx{&vtx, pos++, dep_ಎಣಿಕೆ};
                    m_waitingVertices.emplace(&vtx, newVx);
                }
            } ಅನ್ಯಥಾ {  // REVERSE
                ಯದಿ (vtx.outEmpty()) {
                    ಧ್ರುವ VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.ಸೇರಿಸು(newVx);
                } ಅನ್ಯಥಾ {
                    ಧ್ರುವ uint32_t dep_ಎಣಿಕೆ = vtx.outEdges().size();
                    ಧ್ರುವ VxHolder newVx{&vtx, pos++, dep_ಎಣಿಕೆ};
                    m_waitingVertices.emplace(&vtx, newVx);
                }
            }
        }
    }
    ~GraphStream() = ಸ್ವಭಾವ;

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
    ಧ್ರುವ V3GraphVertex* nextp() {
        ಧ್ರುವ V3GraphVertex* resultp = nullptr;

        typename ReadyVertices::iterator curIt;
        ಯದಿ (m_last == m_readyVertices.end()) {
            // First call to nextp()
            curIt = m_readyVertices.begin();
        } ಅನ್ಯಥಾ {
            // Subsequent call to nextp()
            curIt = m_last;
            ++curIt;
            // Remove previously-returned element
            m_readyVertices.erase(m_last);
            // Wrap curIt. Expect to wrap, and make another pass, to find
            // newly-ready elements that could have appeared ahead of the
            // m_last iterator
            ಯದಿ (curIt == m_readyVertices.end()) curIt = m_readyVertices.begin();
        }

        ಯದಿ (curIt != m_readyVertices.end()) {
            resultp = curIt->vertexp();
            unblockDeps(resultp);
        } ಅನ್ಯಥಾ {
            // No ready vertices; waiting should be empty too, otherwise we
            // were fed a graph with cycles (which is not supported.)
            UASSERT(m_waitingVertices.empty(), "DGS fed non-DAG");
        }

        m_last = curIt;
        ಪ್ರತ್ಯಾಗಮ resultp;
    }

ಗುಪ್ತ:
    ಶೂನ್ಯ unblockDeps(ಧ್ರುವ V3GraphVertex* vertexp) {
        ಯದಿ (m_way == GraphWay::FORWARD) {
            ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edgep : vertexp->outEdges()) {
                ಧ್ರುವ V3GraphVertex* ಧ್ರುವ toVertexp = edgep.ತುದಿ();

                ಧ್ರುವ auto it = m_waitingVertices.find(toVertexp);
                UASSERT_OBJ(it != m_waitingVertices.end(), toVertexp,
                            "Found edge into vertex not in waiting list.");
                ಯದಿ (it->second.unblock()) {
                    m_readyVertices.ಸೇರಿಸು(it->second);
                    m_waitingVertices.erase(it);
                }
            }
        } ಅನ್ಯಥಾ {
            ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : vertexp->inEdges()) {
                ಧ್ರುವ V3GraphVertex* ಧ್ರುವ fromVertexp = edge.fromp();

                ಧ್ರುವ auto it = m_waitingVertices.find(fromVertexp);
                UASSERT_OBJ(it != m_waitingVertices.end(), fromVertexp,
                            "Found edge into vertex not in waiting list.");
                ಯದಿ (it->second.unblock()) {
                    m_readyVertices.ಸೇರಿಸು(it->second);
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
    ಧ್ರುವ GraphWay m_way;  // Direction of traversal
    size_t m_nextIndex = 0;  // Which index to return from m_nextVertices next
    std::vector<ಧ್ರುವ V3GraphVertex*> m_nextVertices;  // List of ready vertices returned next
    std::vector<ಧ್ರುವ V3GraphVertex*> m_readyVertices;  // List of other ready vertices

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    VL_UNCOPYABLE(GraphStreamUnordered);
    explicit GraphStreamUnordered(V3Graph* graphp, GraphWay way = GraphWay::FORWARD)
        : m_way{way} {
        ಯದಿ (m_way == GraphWay::FORWARD) {
            init<GraphWay::FORWARD>(graphp);
        } ಅನ್ಯಥಾ {
            init<GraphWay::REVERSE>(graphp);
        }
    }
    ~GraphStreamUnordered() = ಸ್ವಭಾವ;

    // METHODS

    // Each call to nextp() returns a unique vertex in the graph, in dependency order. Dependencies
    // alone do not specify a total ordering. Un-ordered vertices are returned in an arbitrary but
    // deterministic order.
    ಧ್ರುವ V3GraphVertex* nextp() {
        ಯದಿ (VL_UNLIKELY(m_nextIndex == m_nextVertices.size())) {
            ಯದಿ (VL_UNLIKELY(m_readyVertices.empty())) ಪ್ರತ್ಯಾಗಮ nullptr;
            m_nextIndex = 0;
            // Use swap to avoid reallocation
            m_nextVertices.swap(m_readyVertices);
            m_readyVertices.clear();
        }
        ಧ್ರುವ V3GraphVertex* ಧ್ರುವ resultp = m_nextVertices[m_nextIndex++];
        ಯದಿ (m_way == GraphWay::FORWARD) ಪ್ರತ್ಯಾಗಮ unblock<GraphWay::FORWARD>(resultp);
        ಪ್ರತ್ಯಾಗಮ unblock<GraphWay::REVERSE>(resultp);
    }

ಗುಪ್ತ:
    template <uint8_t N_Way>  //
    VL_ATTR_NOINLINE ಶೂನ್ಯ init(V3Graph* graphp) {
        constexpr GraphWay way{N_Way};
        // Assign every vertex without an incoming edge to ready, others to waiting
        ಪ್ರತಿ (V3GraphVertex& vertex : graphp->vertices()) {
            ಧ್ರುವ uint32_t nDeps = vertex.edges<way.invert()>().size();
            vertex.color(nDeps);  // Using color instead of user, as user might be used by client
            ಯದಿ (VL_UNLIKELY(nDeps == 0)) m_nextVertices.ತಳ್ಳು_back(&vertex);
        }
    }

    template <uint8_t N_Way>  //
    VL_ATTR_NOINLINE ಧ್ರುವ V3GraphVertex* unblock(ಧ್ರುವ V3GraphVertex* resultp) {
        constexpr GraphWay way{N_Way};
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : resultp->edges<way>()) {
            V3GraphVertex* ಧ್ರುವ vertexp = edge.furtherp<way>();
#ಯದಿ VL_DEBUG
            UASSERT_OBJ(vertexp->color() != 0, vertexp, "Should not be on waiting list");
#endif
            vertexp->color(vertexp->color() - 1);
            ಯದಿ (!vertexp->color()) m_readyVertices.ತಳ್ಳು_back(vertexp);
        }
        ಪ್ರತ್ಯಾಗಮ resultp;  // Returning input so we can tail call this method
    }
};

#endif  // Guard
