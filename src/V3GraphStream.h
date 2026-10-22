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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Graph.h"

#അന്തര്ഭാവ <functional>
#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <vector>

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
    പൊതു:
        // MEMBERS
        സ്ഥിരം V3GraphVertex* സ്ഥിരം m_vxp;  // [mtask] Vertex
        സ്ഥിരം uint32_t m_pos;  // Sort position
        uint32_t m_അക്കം_Blocking_Edges;  // Number of blocking edges
        // CONSTRUCTORS
        VxHolder(സ്ഥിരം V3GraphVertex* vxp, uint32_t pos, uint32_t അക്കം_Blocking_Edges)
            : m_vxp{vxp}
            , m_pos{pos}
            , m_അക്കം_Blocking_Edges{അക്കം_Blocking_Edges} {}
        // METHODS
        സ്ഥിരം V3GraphVertex* vertexp() സ്ഥിരം { തിരികെ m_vxp; }
        // Decrement blocking edges count, return true if the vertex is
        // newly unblocked
        bool unblock() {
            UASSERT_OBJ(m_അക്കം_Blocking_Edges > 0, vertexp(), "Underflow of blocking edges");
            --m_അക്കം_Blocking_Edges;
            തിരികെ (m_അക്കം_Blocking_Edges == 0);
        }
    };

    class VxHolderCmp final {
    പൊതു:
        // MEMBERS
        സ്ഥിരം T_Compare m_lessThan;  // Sorting functor
        // CONSTRUCTORS
        explicit VxHolderCmp(സ്ഥിരം T_Compare& lessThan)
            : m_lessThan{lessThan} {}
        // METHODS
        bool operator()(സ്ഥിരം VxHolder& a, സ്ഥിരം VxHolder& b) സ്ഥിരം {
            എങ്കിൽ (m_lessThan.operator()(a.vertexp(), b.vertexp())) തിരികെ true;
            എങ്കിൽ (m_lessThan.operator()(b.vertexp(), a.vertexp())) തിരികെ false;
            തിരികെ a.m_pos < b.m_pos;
        }

    സ്വകാര്യ:
        VL_UNCOPYABLE(VxHolderCmp);
    };

    using ReadyVertices = std::set<VxHolder, VxHolderCmp&>;

    // MEMBERS
    VxHolderCmp m_vxHolderCmp;  // Vertext comparison functor
    ReadyVertices m_readyVertices;  // List of ready vertices
    std::map<സ്ഥിരം V3GraphVertex*, VxHolder> m_waitingVertices;  // List of waiting vertices
    typename ReadyVertices::iterator m_last;  // Previously returned element
    സ്ഥിരം GraphWay m_way;  // FORWARD or REVERSE order of traversal

പൊതു:
    // CONSTRUCTORS
    explicit GraphStream(സ്ഥിരം V3Graph* graphp, GraphWay way = GraphWay::FORWARD,
                         സ്ഥിരം T_Compare& lessThan = T_Compare())
        // NOTE: Perhaps REVERSE way should also reverse the sense of the
        // lessThan function? For now the only usage of REVERSE is not
        // sensitive to its lessThan at all, so it doesn't matter.
        : m_vxHolderCmp{lessThan}
        , m_readyVertices{m_vxHolderCmp}
        , m_last{m_readyVertices.end()}
        , m_way{way} {
        uint32_t pos = 0;
        വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : graphp->vertices()) {
            // Every vertex initially is waiting, or ready.
            എങ്കിൽ (way == GraphWay::FORWARD) {
                എങ്കിൽ (vtx.inEmpty()) {
                    സ്ഥിരം VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.തിരുകുക(newVx);
                } അല്ലെങ്കിൽ {
                    സ്ഥിരം uint32_t dep_എണ്ണം = vtx.inEdges().size();
                    സ്ഥിരം VxHolder newVx{&vtx, pos++, dep_എണ്ണം};
                    m_waitingVertices.emplace(&vtx, newVx);
                }
            } അല്ലെങ്കിൽ {  // REVERSE
                എങ്കിൽ (vtx.outEmpty()) {
                    സ്ഥിരം VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.തിരുകുക(newVx);
                } അല്ലെങ്കിൽ {
                    സ്ഥിരം uint32_t dep_എണ്ണം = vtx.outEdges().size();
                    സ്ഥിരം VxHolder newVx{&vtx, pos++, dep_എണ്ണം};
                    m_waitingVertices.emplace(&vtx, newVx);
                }
            }
        }
    }
    ~GraphStream() = സ്ഥിരസ്ഥിതി;

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
    സ്ഥിരം V3GraphVertex* nextp() {
        സ്ഥിരം V3GraphVertex* resultp = nullptr;

        typename ReadyVertices::iterator curIt;
        എങ്കിൽ (m_last == m_readyVertices.end()) {
            // First call to nextp()
            curIt = m_readyVertices.begin();
        } അല്ലെങ്കിൽ {
            // Subsequent call to nextp()
            curIt = m_last;
            ++curIt;
            // Remove previously-returned element
            m_readyVertices.erase(m_last);
            // Wrap curIt. Expect to wrap, and make another pass, to find
            // newly-ready elements that could have appeared ahead of the
            // m_last iterator
            എങ്കിൽ (curIt == m_readyVertices.end()) curIt = m_readyVertices.begin();
        }

        എങ്കിൽ (curIt != m_readyVertices.end()) {
            resultp = curIt->vertexp();
            unblockDeps(resultp);
        } അല്ലെങ്കിൽ {
            // No ready vertices; waiting should be empty too, otherwise we
            // were fed a graph with cycles (which is not supported.)
            UASSERT(m_waitingVertices.empty(), "DGS fed non-DAG");
        }

        m_last = curIt;
        തിരികെ resultp;
    }

സ്വകാര്യ:
    ശൂന്യം unblockDeps(സ്ഥിരം V3GraphVertex* vertexp) {
        എങ്കിൽ (m_way == GraphWay::FORWARD) {
            വേണ്ടി (സ്ഥിരം V3GraphEdge& edgep : vertexp->outEdges()) {
                സ്ഥിരം V3GraphVertex* സ്ഥിരം toVertexp = edgep.മുകൾ();

                സ്ഥിരം auto it = m_waitingVertices.find(toVertexp);
                UASSERT_OBJ(it != m_waitingVertices.end(), toVertexp,
                            "Found edge into vertex not in waiting list.");
                എങ്കിൽ (it->second.unblock()) {
                    m_readyVertices.തിരുകുക(it->second);
                    m_waitingVertices.erase(it);
                }
            }
        } അല്ലെങ്കിൽ {
            വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : vertexp->inEdges()) {
                സ്ഥിരം V3GraphVertex* സ്ഥിരം fromVertexp = edge.fromp();

                സ്ഥിരം auto it = m_waitingVertices.find(fromVertexp);
                UASSERT_OBJ(it != m_waitingVertices.end(), fromVertexp,
                            "Found edge into vertex not in waiting list.");
                എങ്കിൽ (it->second.unblock()) {
                    m_readyVertices.തിരുകുക(it->second);
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
    സ്ഥിരം GraphWay m_way;  // Direction of traversal
    size_t m_nextIndex = 0;  // Which index to return from m_nextVertices next
    std::vector<സ്ഥിരം V3GraphVertex*> m_nextVertices;  // List of ready vertices returned next
    std::vector<സ്ഥിരം V3GraphVertex*> m_readyVertices;  // List of other ready vertices

പൊതു:
    // CONSTRUCTORS
    VL_UNCOPYABLE(GraphStreamUnordered);
    explicit GraphStreamUnordered(V3Graph* graphp, GraphWay way = GraphWay::FORWARD)
        : m_way{way} {
        എങ്കിൽ (m_way == GraphWay::FORWARD) {
            init<GraphWay::FORWARD>(graphp);
        } അല്ലെങ്കിൽ {
            init<GraphWay::REVERSE>(graphp);
        }
    }
    ~GraphStreamUnordered() = സ്ഥിരസ്ഥിതി;

    // METHODS

    // Each call to nextp() returns a unique vertex in the graph, in dependency order. Dependencies
    // alone do not specify a total ordering. Un-ordered vertices are returned in an arbitrary but
    // deterministic order.
    സ്ഥിരം V3GraphVertex* nextp() {
        എങ്കിൽ (VL_UNLIKELY(m_nextIndex == m_nextVertices.size())) {
            എങ്കിൽ (VL_UNLIKELY(m_readyVertices.empty())) തിരികെ nullptr;
            m_nextIndex = 0;
            // Use swap to avoid reallocation
            m_nextVertices.swap(m_readyVertices);
            m_readyVertices.clear();
        }
        സ്ഥിരം V3GraphVertex* സ്ഥിരം resultp = m_nextVertices[m_nextIndex++];
        എങ്കിൽ (m_way == GraphWay::FORWARD) തിരികെ unblock<GraphWay::FORWARD>(resultp);
        തിരികെ unblock<GraphWay::REVERSE>(resultp);
    }

സ്വകാര്യ:
    template <uint8_t N_Way>  //
    VL_ATTR_NOINLINE ശൂന്യം init(V3Graph* graphp) {
        constexpr GraphWay way{N_Way};
        // Assign every vertex without an incoming edge to ready, others to waiting
        വേണ്ടി (V3GraphVertex& vertex : graphp->vertices()) {
            സ്ഥിരം uint32_t nDeps = vertex.edges<way.invert()>().size();
            vertex.color(nDeps);  // Using color instead of user, as user might be used by client
            എങ്കിൽ (VL_UNLIKELY(nDeps == 0)) m_nextVertices.തള്ളുക_back(&vertex);
        }
    }

    template <uint8_t N_Way>  //
    VL_ATTR_NOINLINE സ്ഥിരം V3GraphVertex* unblock(സ്ഥിരം V3GraphVertex* resultp) {
        constexpr GraphWay way{N_Way};
        വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : resultp->edges<way>()) {
            V3GraphVertex* സ്ഥിരം vertexp = edge.furtherp<way>();
#എങ്കിൽ VL_DEBUG
            UASSERT_OBJ(vertexp->color() != 0, vertexp, "Should not be on waiting list");
#endif
            vertexp->color(vertexp->color() - 1);
            എങ്കിൽ (!vertexp->color()) m_readyVertices.തള്ളുക_back(vertexp);
        }
        തിരികെ resultp;  // Returning input so we can tail call this method
    }
};

#endif  // Guard
