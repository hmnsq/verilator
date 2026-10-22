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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Graph.h"

#અન્તર્ભાવ <functional>
#અન્તર્ભાવ <map>
#અન્તર્ભાવ <set>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <vector>

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
    જાહેર:
        // MEMBERS
        અચલ V3GraphVertex* અચલ m_vxp;  // [mtask] Vertex
        અચલ uint32_t m_pos;  // Sort position
        uint32_t m_અંક_Blocking_Edges;  // Number of blocking edges
        // CONSTRUCTORS
        VxHolder(અચલ V3GraphVertex* vxp, uint32_t pos, uint32_t અંક_Blocking_Edges)
            : m_vxp{vxp}
            , m_pos{pos}
            , m_અંક_Blocking_Edges{અંક_Blocking_Edges} {}
        // METHODS
        અચલ V3GraphVertex* vertexp() અચલ { પાછા m_vxp; }
        // Decrement blocking edges count, return true if the vertex is
        // newly unblocked
        bool unblock() {
            UASSERT_OBJ(m_અંક_Blocking_Edges > 0, vertexp(), "Underflow of blocking edges");
            --m_અંક_Blocking_Edges;
            પાછા (m_અંક_Blocking_Edges == 0);
        }
    };

    class VxHolderCmp final {
    જાહેર:
        // MEMBERS
        અચલ T_Compare m_lessThan;  // Sorting functor
        // CONSTRUCTORS
        explicit VxHolderCmp(અચલ T_Compare& lessThan)
            : m_lessThan{lessThan} {}
        // METHODS
        bool operator()(અચલ VxHolder& a, અચલ VxHolder& b) અચલ {
            જો (m_lessThan.operator()(a.vertexp(), b.vertexp())) પાછા true;
            જો (m_lessThan.operator()(b.vertexp(), a.vertexp())) પાછા false;
            પાછા a.m_pos < b.m_pos;
        }

    ખાનગી:
        VL_UNCOPYABLE(VxHolderCmp);
    };

    using ReadyVertices = std::set<VxHolder, VxHolderCmp&>;

    // MEMBERS
    VxHolderCmp m_vxHolderCmp;  // Vertext comparison functor
    ReadyVertices m_readyVertices;  // List of ready vertices
    std::map<અચલ V3GraphVertex*, VxHolder> m_waitingVertices;  // List of waiting vertices
    typename ReadyVertices::iterator m_last;  // Previously returned element
    અચલ GraphWay m_way;  // FORWARD or REVERSE order of traversal

જાહેર:
    // CONSTRUCTORS
    explicit GraphStream(અચલ V3Graph* graphp, GraphWay way = GraphWay::FORWARD,
                         અચલ T_Compare& lessThan = T_Compare())
        // NOTE: Perhaps REVERSE way should also reverse the sense of the
        // lessThan function? For now the only usage of REVERSE is not
        // sensitive to its lessThan at all, so it doesn't matter.
        : m_vxHolderCmp{lessThan}
        , m_readyVertices{m_vxHolderCmp}
        , m_last{m_readyVertices.end()}
        , m_way{way} {
        uint32_t pos = 0;
        માટે (અચલ V3GraphVertex& vtx : graphp->vertices()) {
            // Every vertex initially is waiting, or ready.
            જો (way == GraphWay::FORWARD) {
                જો (vtx.inEmpty()) {
                    અચલ VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.ઉમેરો(newVx);
                } નહીંતર {
                    અચલ uint32_t dep_ગણતરી = vtx.inEdges().size();
                    અચલ VxHolder newVx{&vtx, pos++, dep_ગણતરી};
                    m_waitingVertices.emplace(&vtx, newVx);
                }
            } નહીંતર {  // REVERSE
                જો (vtx.outEmpty()) {
                    અચલ VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.ઉમેરો(newVx);
                } નહીંતર {
                    અચલ uint32_t dep_ગણતરી = vtx.outEdges().size();
                    અચલ VxHolder newVx{&vtx, pos++, dep_ગણતરી};
                    m_waitingVertices.emplace(&vtx, newVx);
                }
            }
        }
    }
    ~GraphStream() = મૂળભૂત;

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
    અચલ V3GraphVertex* nextp() {
        અચલ V3GraphVertex* resultp = nullptr;

        typename ReadyVertices::iterator curIt;
        જો (m_last == m_readyVertices.end()) {
            // First call to nextp()
            curIt = m_readyVertices.begin();
        } નહીંતર {
            // Subsequent call to nextp()
            curIt = m_last;
            ++curIt;
            // Remove previously-returned element
            m_readyVertices.erase(m_last);
            // Wrap curIt. Expect to wrap, and make another pass, to find
            // newly-ready elements that could have appeared ahead of the
            // m_last iterator
            જો (curIt == m_readyVertices.end()) curIt = m_readyVertices.begin();
        }

        જો (curIt != m_readyVertices.end()) {
            resultp = curIt->vertexp();
            unblockDeps(resultp);
        } નહીંતર {
            // No ready vertices; waiting should be empty too, otherwise we
            // were fed a graph with cycles (which is not supported.)
            UASSERT(m_waitingVertices.empty(), "DGS fed non-DAG");
        }

        m_last = curIt;
        પાછા resultp;
    }

ખાનગી:
    શૂન્ય unblockDeps(અચલ V3GraphVertex* vertexp) {
        જો (m_way == GraphWay::FORWARD) {
            માટે (અચલ V3GraphEdge& edgep : vertexp->outEdges()) {
                અચલ V3GraphVertex* અચલ toVertexp = edgep.ટોચ();

                અચલ auto it = m_waitingVertices.find(toVertexp);
                UASSERT_OBJ(it != m_waitingVertices.end(), toVertexp,
                            "Found edge into vertex not in waiting list.");
                જો (it->second.unblock()) {
                    m_readyVertices.ઉમેરો(it->second);
                    m_waitingVertices.erase(it);
                }
            }
        } નહીંતર {
            માટે (અચલ V3GraphEdge& edge : vertexp->inEdges()) {
                અચલ V3GraphVertex* અચલ fromVertexp = edge.fromp();

                અચલ auto it = m_waitingVertices.find(fromVertexp);
                UASSERT_OBJ(it != m_waitingVertices.end(), fromVertexp,
                            "Found edge into vertex not in waiting list.");
                જો (it->second.unblock()) {
                    m_readyVertices.ઉમેરો(it->second);
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
    અચલ GraphWay m_way;  // Direction of traversal
    size_t m_nextIndex = 0;  // Which index to return from m_nextVertices next
    std::vector<અચલ V3GraphVertex*> m_nextVertices;  // List of ready vertices returned next
    std::vector<અચલ V3GraphVertex*> m_readyVertices;  // List of other ready vertices

જાહેર:
    // CONSTRUCTORS
    VL_UNCOPYABLE(GraphStreamUnordered);
    explicit GraphStreamUnordered(V3Graph* graphp, GraphWay way = GraphWay::FORWARD)
        : m_way{way} {
        જો (m_way == GraphWay::FORWARD) {
            init<GraphWay::FORWARD>(graphp);
        } નહીંતર {
            init<GraphWay::REVERSE>(graphp);
        }
    }
    ~GraphStreamUnordered() = મૂળભૂત;

    // METHODS

    // Each call to nextp() returns a unique vertex in the graph, in dependency order. Dependencies
    // alone do not specify a total ordering. Un-ordered vertices are returned in an arbitrary but
    // deterministic order.
    અચલ V3GraphVertex* nextp() {
        જો (VL_UNLIKELY(m_nextIndex == m_nextVertices.size())) {
            જો (VL_UNLIKELY(m_readyVertices.empty())) પાછા nullptr;
            m_nextIndex = 0;
            // Use swap to avoid reallocation
            m_nextVertices.swap(m_readyVertices);
            m_readyVertices.clear();
        }
        અચલ V3GraphVertex* અચલ resultp = m_nextVertices[m_nextIndex++];
        જો (m_way == GraphWay::FORWARD) પાછા unblock<GraphWay::FORWARD>(resultp);
        પાછા unblock<GraphWay::REVERSE>(resultp);
    }

ખાનગી:
    template <uint8_t N_Way>  //
    VL_ATTR_NOINLINE શૂન્ય init(V3Graph* graphp) {
        constexpr GraphWay way{N_Way};
        // Assign every vertex without an incoming edge to ready, others to waiting
        માટે (V3GraphVertex& vertex : graphp->vertices()) {
            અચલ uint32_t nDeps = vertex.edges<way.invert()>().size();
            vertex.color(nDeps);  // Using color instead of user, as user might be used by client
            જો (VL_UNLIKELY(nDeps == 0)) m_nextVertices.ધકેલો_back(&vertex);
        }
    }

    template <uint8_t N_Way>  //
    VL_ATTR_NOINLINE અચલ V3GraphVertex* unblock(અચલ V3GraphVertex* resultp) {
        constexpr GraphWay way{N_Way};
        માટે (અચલ V3GraphEdge& edge : resultp->edges<way>()) {
            V3GraphVertex* અચલ vertexp = edge.furtherp<way>();
#જો VL_DEBUG
            UASSERT_OBJ(vertexp->color() != 0, vertexp, "Should not be on waiting list");
#endif
            vertexp->color(vertexp->color() - 1);
            જો (!vertexp->color()) m_readyVertices.ધકેલો_back(vertexp);
        }
        પાછા resultp;  // Returning input so we can tail call this method
    }
};

#endif  // Guard
