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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Graph.h"

#அந்தர்பாவ <functional>
#அந்தர்பாவ <map>
#அந்தர்பாவ <set>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <vector>

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
    பொது:
        // MEMBERS
        மாறிலி V3GraphVertex* மாறிலி m_vxp;  // [mtask] Vertex
        மாறிலி uint32_t m_pos;  // Sort position
        uint32_t m_இலக்கம்_Blocking_Edges;  // Number of blocking edges
        // CONSTRUCTORS
        VxHolder(மாறிலி V3GraphVertex* vxp, uint32_t pos, uint32_t இலக்கம்_Blocking_Edges)
            : m_vxp{vxp}
            , m_pos{pos}
            , m_இலக்கம்_Blocking_Edges{இலக்கம்_Blocking_Edges} {}
        // METHODS
        மாறிலி V3GraphVertex* vertexp() மாறிலி { திரும்பு m_vxp; }
        // Decrement blocking edges count, return true if the vertex is
        // newly unblocked
        bool unblock() {
            UASSERT_OBJ(m_இலக்கம்_Blocking_Edges > 0, vertexp(), "Underflow of blocking edges");
            --m_இலக்கம்_Blocking_Edges;
            திரும்பு (m_இலக்கம்_Blocking_Edges == 0);
        }
    };

    class VxHolderCmp final {
    பொது:
        // MEMBERS
        மாறிலி T_Compare m_lessThan;  // Sorting functor
        // CONSTRUCTORS
        explicit VxHolderCmp(மாறிலி T_Compare& lessThan)
            : m_lessThan{lessThan} {}
        // METHODS
        bool operator()(மாறிலி VxHolder& a, மாறிலி VxHolder& b) மாறிலி {
            எனில் (m_lessThan.operator()(a.vertexp(), b.vertexp())) திரும்பு true;
            எனில் (m_lessThan.operator()(b.vertexp(), a.vertexp())) திரும்பு false;
            திரும்பு a.m_pos < b.m_pos;
        }

    தனிப்பட்ட:
        VL_UNCOPYABLE(VxHolderCmp);
    };

    using ReadyVertices = std::set<VxHolder, VxHolderCmp&>;

    // MEMBERS
    VxHolderCmp m_vxHolderCmp;  // Vertext comparison functor
    ReadyVertices m_readyVertices;  // List of ready vertices
    std::map<மாறிலி V3GraphVertex*, VxHolder> m_waitingVertices;  // List of waiting vertices
    typename ReadyVertices::iterator m_last;  // Previously returned element
    மாறிலி GraphWay m_way;  // FORWARD or REVERSE order of traversal

பொது:
    // CONSTRUCTORS
    explicit GraphStream(மாறிலி V3Graph* graphp, GraphWay way = GraphWay::FORWARD,
                         மாறிலி T_Compare& lessThan = T_Compare())
        // NOTE: Perhaps REVERSE way should also reverse the sense of the
        // lessThan function? For now the only usage of REVERSE is not
        // sensitive to its lessThan at all, so it doesn't matter.
        : m_vxHolderCmp{lessThan}
        , m_readyVertices{m_vxHolderCmp}
        , m_last{m_readyVertices.end()}
        , m_way{way} {
        uint32_t pos = 0;
        க்கு (மாறிலி V3GraphVertex& vtx : graphp->vertices()) {
            // Every vertex initially is waiting, or ready.
            எனில் (way == GraphWay::FORWARD) {
                எனில் (vtx.inEmpty()) {
                    மாறிலி VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.செருகு(newVx);
                } இல்லையெனில் {
                    மாறிலி uint32_t dep_மொத்தம் = vtx.inEdges().size();
                    மாறிலி VxHolder newVx{&vtx, pos++, dep_மொத்தம்};
                    m_waitingVertices.emplace(&vtx, newVx);
                }
            } இல்லையெனில் {  // REVERSE
                எனில் (vtx.outEmpty()) {
                    மாறிலி VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.செருகு(newVx);
                } இல்லையெனில் {
                    மாறிலி uint32_t dep_மொத்தம் = vtx.outEdges().size();
                    மாறிலி VxHolder newVx{&vtx, pos++, dep_மொத்தம்};
                    m_waitingVertices.emplace(&vtx, newVx);
                }
            }
        }
    }
    ~GraphStream() = இயல்பு;

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
    மாறிலி V3GraphVertex* nextp() {
        மாறிலி V3GraphVertex* resultp = nullptr;

        typename ReadyVertices::iterator curIt;
        எனில் (m_last == m_readyVertices.end()) {
            // First call to nextp()
            curIt = m_readyVertices.begin();
        } இல்லையெனில் {
            // Subsequent call to nextp()
            curIt = m_last;
            ++curIt;
            // Remove previously-returned element
            m_readyVertices.erase(m_last);
            // Wrap curIt. Expect to wrap, and make another pass, to find
            // newly-ready elements that could have appeared ahead of the
            // m_last iterator
            எனில் (curIt == m_readyVertices.end()) curIt = m_readyVertices.begin();
        }

        எனில் (curIt != m_readyVertices.end()) {
            resultp = curIt->vertexp();
            unblockDeps(resultp);
        } இல்லையெனில் {
            // No ready vertices; waiting should be empty too, otherwise we
            // were fed a graph with cycles (which is not supported.)
            UASSERT(m_waitingVertices.empty(), "DGS fed non-DAG");
        }

        m_last = curIt;
        திரும்பு resultp;
    }

தனிப்பட்ட:
    வெறுமை unblockDeps(மாறிலி V3GraphVertex* vertexp) {
        எனில் (m_way == GraphWay::FORWARD) {
            க்கு (மாறிலி V3GraphEdge& edgep : vertexp->outEdges()) {
                மாறிலி V3GraphVertex* மாறிலி toVertexp = edgep.உச்சி();

                மாறிலி auto it = m_waitingVertices.find(toVertexp);
                UASSERT_OBJ(it != m_waitingVertices.end(), toVertexp,
                            "Found edge into vertex not in waiting list.");
                எனில் (it->second.unblock()) {
                    m_readyVertices.செருகு(it->second);
                    m_waitingVertices.erase(it);
                }
            }
        } இல்லையெனில் {
            க்கு (மாறிலி V3GraphEdge& edge : vertexp->inEdges()) {
                மாறிலி V3GraphVertex* மாறிலி fromVertexp = edge.fromp();

                மாறிலி auto it = m_waitingVertices.find(fromVertexp);
                UASSERT_OBJ(it != m_waitingVertices.end(), fromVertexp,
                            "Found edge into vertex not in waiting list.");
                எனில் (it->second.unblock()) {
                    m_readyVertices.செருகு(it->second);
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
    மாறிலி GraphWay m_way;  // Direction of traversal
    size_t m_nextIndex = 0;  // Which index to return from m_nextVertices next
    std::vector<மாறிலி V3GraphVertex*> m_nextVertices;  // List of ready vertices returned next
    std::vector<மாறிலி V3GraphVertex*> m_readyVertices;  // List of other ready vertices

பொது:
    // CONSTRUCTORS
    VL_UNCOPYABLE(GraphStreamUnordered);
    explicit GraphStreamUnordered(V3Graph* graphp, GraphWay way = GraphWay::FORWARD)
        : m_way{way} {
        எனில் (m_way == GraphWay::FORWARD) {
            init<GraphWay::FORWARD>(graphp);
        } இல்லையெனில் {
            init<GraphWay::REVERSE>(graphp);
        }
    }
    ~GraphStreamUnordered() = இயல்பு;

    // METHODS

    // Each call to nextp() returns a unique vertex in the graph, in dependency order. Dependencies
    // alone do not specify a total ordering. Un-ordered vertices are returned in an arbitrary but
    // deterministic order.
    மாறிலி V3GraphVertex* nextp() {
        எனில் (VL_UNLIKELY(m_nextIndex == m_nextVertices.size())) {
            எனில் (VL_UNLIKELY(m_readyVertices.empty())) திரும்பு nullptr;
            m_nextIndex = 0;
            // Use swap to avoid reallocation
            m_nextVertices.swap(m_readyVertices);
            m_readyVertices.clear();
        }
        மாறிலி V3GraphVertex* மாறிலி resultp = m_nextVertices[m_nextIndex++];
        எனில் (m_way == GraphWay::FORWARD) திரும்பு unblock<GraphWay::FORWARD>(resultp);
        திரும்பு unblock<GraphWay::REVERSE>(resultp);
    }

தனிப்பட்ட:
    template <uint8_t N_Way>  //
    VL_ATTR_NOINLINE வெறுமை init(V3Graph* graphp) {
        constexpr GraphWay way{N_Way};
        // Assign every vertex without an incoming edge to ready, others to waiting
        க்கு (V3GraphVertex& vertex : graphp->vertices()) {
            மாறிலி uint32_t nDeps = vertex.edges<way.invert()>().size();
            vertex.color(nDeps);  // Using color instead of user, as user might be used by client
            எனில் (VL_UNLIKELY(nDeps == 0)) m_nextVertices.தள்ளு_back(&vertex);
        }
    }

    template <uint8_t N_Way>  //
    VL_ATTR_NOINLINE மாறிலி V3GraphVertex* unblock(மாறிலி V3GraphVertex* resultp) {
        constexpr GraphWay way{N_Way};
        க்கு (மாறிலி V3GraphEdge& edge : resultp->edges<way>()) {
            V3GraphVertex* மாறிலி vertexp = edge.furtherp<way>();
#எனில் VL_DEBUG
            UASSERT_OBJ(vertexp->color() != 0, vertexp, "Should not be on waiting list");
#endif
            vertexp->color(vertexp->color() - 1);
            எனில் (!vertexp->color()) m_readyVertices.தள்ளு_back(vertexp);
        }
        திரும்பு resultp;  // Returning input so we can tail call this method
    }
};

#endif  // Guard
