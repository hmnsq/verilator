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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Graph.h"

#ଅନ୍ତର୍ଭାଵ <functional>
#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <vector>

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
    ସର୍ବସାଧାରଣ:
        // MEMBERS
        ଅଚଳ V3GraphVertex* ଅଚଳ m_vxp;  // [mtask] Vertex
        ଅଚଳ uint32_t m_pos;  // Sort position
        uint32_t m_ଅଙ୍କ_Blocking_Edges;  // Number of blocking edges
        // CONSTRUCTORS
        VxHolder(ଅଚଳ V3GraphVertex* vxp, uint32_t pos, uint32_t ଅଙ୍କ_Blocking_Edges)
            : m_vxp{vxp}
            , m_pos{pos}
            , m_ଅଙ୍କ_Blocking_Edges{ଅଙ୍କ_Blocking_Edges} {}
        // METHODS
        ଅଚଳ V3GraphVertex* vertexp() ଅଚଳ { ଫେରନ୍ତୁ m_vxp; }
        // Decrement blocking edges count, return true if the vertex is
        // newly unblocked
        bool unblock() {
            UASSERT_OBJ(m_ଅଙ୍କ_Blocking_Edges > 0, vertexp(), "Underflow of blocking edges");
            --m_ଅଙ୍କ_Blocking_Edges;
            ଫେରନ୍ତୁ (m_ଅଙ୍କ_Blocking_Edges == 0);
        }
    };

    class VxHolderCmp final {
    ସର୍ବସାଧାରଣ:
        // MEMBERS
        ଅଚଳ T_Compare m_lessThan;  // Sorting functor
        // CONSTRUCTORS
        explicit VxHolderCmp(ଅଚଳ T_Compare& lessThan)
            : m_lessThan{lessThan} {}
        // METHODS
        bool operator()(ଅଚଳ VxHolder& a, ଅଚଳ VxHolder& b) ଅଚଳ {
            ଯଦି (m_lessThan.operator()(a.vertexp(), b.vertexp())) ଫେରନ୍ତୁ true;
            ଯଦି (m_lessThan.operator()(b.vertexp(), a.vertexp())) ଫେରନ୍ତୁ false;
            ଫେରନ୍ତୁ a.m_pos < b.m_pos;
        }

    ବ୍ୟକ୍ତିଗତ:
        VL_UNCOPYABLE(VxHolderCmp);
    };

    using ReadyVertices = std::set<VxHolder, VxHolderCmp&>;

    // MEMBERS
    VxHolderCmp m_vxHolderCmp;  // Vertext comparison functor
    ReadyVertices m_readyVertices;  // List of ready vertices
    std::map<ଅଚଳ V3GraphVertex*, VxHolder> m_waitingVertices;  // List of waiting vertices
    typename ReadyVertices::iterator m_last;  // Previously returned element
    ଅଚଳ GraphWay m_way;  // FORWARD or REVERSE order of traversal

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit GraphStream(ଅଚଳ V3Graph* graphp, GraphWay way = GraphWay::FORWARD,
                         ଅଚଳ T_Compare& lessThan = T_Compare())
        // NOTE: Perhaps REVERSE way should also reverse the sense of the
        // lessThan function? For now the only usage of REVERSE is not
        // sensitive to its lessThan at all, so it doesn't matter.
        : m_vxHolderCmp{lessThan}
        , m_readyVertices{m_vxHolderCmp}
        , m_last{m_readyVertices.end()}
        , m_way{way} {
        uint32_t pos = 0;
        ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : graphp->vertices()) {
            // Every vertex initially is waiting, or ready.
            ଯଦି (way == GraphWay::FORWARD) {
                ଯଦି (vtx.inEmpty()) {
                    ଅଚଳ VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.ଭର୍ତ୍ତି(newVx);
                } ଅନ୍ୟଥା {
                    ଅଚଳ uint32_t dep_ଗଣନା = vtx.inEdges().size();
                    ଅଚଳ VxHolder newVx{&vtx, pos++, dep_ଗଣନା};
                    m_waitingVertices.emplace(&vtx, newVx);
                }
            } ଅନ୍ୟଥା {  // REVERSE
                ଯଦି (vtx.outEmpty()) {
                    ଅଚଳ VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.ଭର୍ତ୍ତି(newVx);
                } ଅନ୍ୟଥା {
                    ଅଚଳ uint32_t dep_ଗଣନା = vtx.outEdges().size();
                    ଅଚଳ VxHolder newVx{&vtx, pos++, dep_ଗଣନା};
                    m_waitingVertices.emplace(&vtx, newVx);
                }
            }
        }
    }
    ~GraphStream() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

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
    ଅଚଳ V3GraphVertex* nextp() {
        ଅଚଳ V3GraphVertex* resultp = nullptr;

        typename ReadyVertices::iterator curIt;
        ଯଦି (m_last == m_readyVertices.end()) {
            // First call to nextp()
            curIt = m_readyVertices.begin();
        } ଅନ୍ୟଥା {
            // Subsequent call to nextp()
            curIt = m_last;
            ++curIt;
            // Remove previously-returned element
            m_readyVertices.erase(m_last);
            // Wrap curIt. Expect to wrap, and make another pass, to find
            // newly-ready elements that could have appeared ahead of the
            // m_last iterator
            ଯଦି (curIt == m_readyVertices.end()) curIt = m_readyVertices.begin();
        }

        ଯଦି (curIt != m_readyVertices.end()) {
            resultp = curIt->vertexp();
            unblockDeps(resultp);
        } ଅନ୍ୟଥା {
            // No ready vertices; waiting should be empty too, otherwise we
            // were fed a graph with cycles (which is not supported.)
            UASSERT(m_waitingVertices.empty(), "DGS fed non-DAG");
        }

        m_last = curIt;
        ଫେରନ୍ତୁ resultp;
    }

ବ୍ୟକ୍ତିଗତ:
    ଶୂନ୍ୟ unblockDeps(ଅଚଳ V3GraphVertex* vertexp) {
        ଯଦି (m_way == GraphWay::FORWARD) {
            ପାଇଁ (ଅଚଳ V3GraphEdge& edgep : vertexp->outEdges()) {
                ଅଚଳ V3GraphVertex* ଅଚଳ toVertexp = edgep.ଶୀର୍ଷ();

                ଅଚଳ auto it = m_waitingVertices.find(toVertexp);
                UASSERT_OBJ(it != m_waitingVertices.end(), toVertexp,
                            "Found edge into vertex not in waiting list.");
                ଯଦି (it->second.unblock()) {
                    m_readyVertices.ଭର୍ତ୍ତି(it->second);
                    m_waitingVertices.erase(it);
                }
            }
        } ଅନ୍ୟଥା {
            ପାଇଁ (ଅଚଳ V3GraphEdge& edge : vertexp->inEdges()) {
                ଅଚଳ V3GraphVertex* ଅଚଳ fromVertexp = edge.fromp();

                ଅଚଳ auto it = m_waitingVertices.find(fromVertexp);
                UASSERT_OBJ(it != m_waitingVertices.end(), fromVertexp,
                            "Found edge into vertex not in waiting list.");
                ଯଦି (it->second.unblock()) {
                    m_readyVertices.ଭର୍ତ୍ତି(it->second);
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
    ଅଚଳ GraphWay m_way;  // Direction of traversal
    size_t m_nextIndex = 0;  // Which index to return from m_nextVertices next
    std::vector<ଅଚଳ V3GraphVertex*> m_nextVertices;  // List of ready vertices returned next
    std::vector<ଅଚଳ V3GraphVertex*> m_readyVertices;  // List of other ready vertices

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    VL_UNCOPYABLE(GraphStreamUnordered);
    explicit GraphStreamUnordered(V3Graph* graphp, GraphWay way = GraphWay::FORWARD)
        : m_way{way} {
        ଯଦି (m_way == GraphWay::FORWARD) {
            init<GraphWay::FORWARD>(graphp);
        } ଅନ୍ୟଥା {
            init<GraphWay::REVERSE>(graphp);
        }
    }
    ~GraphStreamUnordered() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // METHODS

    // Each call to nextp() returns a unique vertex in the graph, in dependency order. Dependencies
    // alone do not specify a total ordering. Un-ordered vertices are returned in an arbitrary but
    // deterministic order.
    ଅଚଳ V3GraphVertex* nextp() {
        ଯଦି (VL_UNLIKELY(m_nextIndex == m_nextVertices.size())) {
            ଯଦି (VL_UNLIKELY(m_readyVertices.empty())) ଫେରନ୍ତୁ nullptr;
            m_nextIndex = 0;
            // Use swap to avoid reallocation
            m_nextVertices.swap(m_readyVertices);
            m_readyVertices.clear();
        }
        ଅଚଳ V3GraphVertex* ଅଚଳ resultp = m_nextVertices[m_nextIndex++];
        ଯଦି (m_way == GraphWay::FORWARD) ଫେରନ୍ତୁ unblock<GraphWay::FORWARD>(resultp);
        ଫେରନ୍ତୁ unblock<GraphWay::REVERSE>(resultp);
    }

ବ୍ୟକ୍ତିଗତ:
    template <uint8_t N_Way>  //
    VL_ATTR_NOINLINE ଶୂନ୍ୟ init(V3Graph* graphp) {
        constexpr GraphWay way{N_Way};
        // Assign every vertex without an incoming edge to ready, others to waiting
        ପାଇଁ (V3GraphVertex& vertex : graphp->vertices()) {
            ଅଚଳ uint32_t nDeps = vertex.edges<way.invert()>().size();
            vertex.color(nDeps);  // Using color instead of user, as user might be used by client
            ଯଦି (VL_UNLIKELY(nDeps == 0)) m_nextVertices.ଠେଲ_back(&vertex);
        }
    }

    template <uint8_t N_Way>  //
    VL_ATTR_NOINLINE ଅଚଳ V3GraphVertex* unblock(ଅଚଳ V3GraphVertex* resultp) {
        constexpr GraphWay way{N_Way};
        ପାଇଁ (ଅଚଳ V3GraphEdge& edge : resultp->edges<way>()) {
            V3GraphVertex* ଅଚଳ vertexp = edge.furtherp<way>();
#ଯଦି VL_DEBUG
            UASSERT_OBJ(vertexp->color() != 0, vertexp, "Should not be on waiting list");
#endif
            vertexp->color(vertexp->color() - 1);
            ଯଦି (!vertexp->color()) m_readyVertices.ଠେଲ_back(vertexp);
        }
        ଫେରନ୍ତୁ resultp;  // Returning input so we can tail call this method
    }
};

#endif  // Guard
