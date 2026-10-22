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

ढाँचा <typename T_Compare>
class GraphStream final {
    // TYPES
    class VxHolder final {
    सार्वजनिक:
        // MEMBERS
        स्थिरांक V3GraphVertex* स्थिरांक m_vxp;  // [mtask] Vertex
        स्थिरांक uint32_t m_pos;  // Sort position
        uint32_t m_अंक_Blocking_Edges;  // Number of blocking edges
        // CONSTRUCTORS
        VxHolder(स्थिरांक V3GraphVertex* vxp, uint32_t pos, uint32_t अंक_Blocking_Edges)
            : m_vxp{vxp}
            , m_pos{pos}
            , m_अंक_Blocking_Edges{अंक_Blocking_Edges} {}
        // METHODS
        स्थिरांक V3GraphVertex* vertexp() स्थिरांक { परत m_vxp; }
        // Decrement blocking edges count, return true if the vertex is
        // newly unblocked
        bool unblock() {
            UASSERT_OBJ(m_अंक_Blocking_Edges > 0, vertexp(), "Underflow of blocking edges");
            --m_अंक_Blocking_Edges;
            परत (m_अंक_Blocking_Edges == 0);
        }
    };

    class VxHolderCmp final {
    सार्वजनिक:
        // MEMBERS
        स्थिरांक T_Compare m_lessThan;  // Sorting functor
        // CONSTRUCTORS
        explicit VxHolderCmp(स्थिरांक T_Compare& lessThan)
            : m_lessThan{lessThan} {}
        // METHODS
        bool चालक()(स्थिरांक VxHolder& a, स्थिरांक VxHolder& b) स्थिरांक {
            जर (m_lessThan.चालक()(a.vertexp(), b.vertexp())) परत true;
            जर (m_lessThan.चालक()(b.vertexp(), a.vertexp())) परत false;
            परत a.m_pos < b.m_pos;
        }

    खाजगी:
        VL_UNCOPYABLE(VxHolderCmp);
    };

    using ReadyVertices = std::set<VxHolder, VxHolderCmp&>;

    // MEMBERS
    VxHolderCmp m_vxHolderCmp;  // Vertext comparison functor
    ReadyVertices m_readyVertices;  // List of ready vertices
    std::map<स्थिरांक V3GraphVertex*, VxHolder> m_waitingVertices;  // List of waiting vertices
    typename ReadyVertices::iterator m_last;  // Previously returned element
    स्थिरांक GraphWay m_way;  // FORWARD or REVERSE order of traversal

सार्वजनिक:
    // CONSTRUCTORS
    explicit GraphStream(स्थिरांक V3Graph* graphp, GraphWay way = GraphWay::FORWARD,
                         स्थिरांक T_Compare& lessThan = T_Compare())
        // NOTE: Perhaps REVERSE way should also reverse the sense of the
        // lessThan function? For now the only usage of REVERSE is not
        // sensitive to its lessThan at all, so it doesn't matter.
        : m_vxHolderCmp{lessThan}
        , m_readyVertices{m_vxHolderCmp}
        , m_last{m_readyVertices.end()}
        , m_way{way} {
        uint32_t pos = 0;
        साठी (स्थिरांक V3GraphVertex& vtx : graphp->vertices()) {
            // Every vertex initially is waiting, or ready.
            जर (way == GraphWay::FORWARD) {
                जर (vtx.inEmpty()) {
                    स्थिरांक VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.घाला(newVx);
                } अन्यथा {
                    स्थिरांक uint32_t dep_मोजणी = vtx.inEdges().size();
                    स्थिरांक VxHolder newVx{&vtx, pos++, dep_मोजणी};
                    m_waitingVertices.emplace(&vtx, newVx);
                }
            } अन्यथा {  // REVERSE
                जर (vtx.outEmpty()) {
                    स्थिरांक VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.घाला(newVx);
                } अन्यथा {
                    स्थिरांक uint32_t dep_मोजणी = vtx.outEdges().size();
                    स्थिरांक VxHolder newVx{&vtx, pos++, dep_मोजणी};
                    m_waitingVertices.emplace(&vtx, newVx);
                }
            }
        }
    }
    ~GraphStream() = पूर्वनिर्धारित;

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
    स्थिरांक V3GraphVertex* nextp() {
        स्थिरांक V3GraphVertex* resultp = nullptr;

        typename ReadyVertices::iterator curIt;
        जर (m_last == m_readyVertices.end()) {
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
            जर (curIt == m_readyVertices.end()) curIt = m_readyVertices.begin();
        }

        जर (curIt != m_readyVertices.end()) {
            resultp = curIt->vertexp();
            unblockDeps(resultp);
        } अन्यथा {
            // No ready vertices; waiting should be empty too, otherwise we
            // were fed a graph with cycles (which is not supported.)
            UASSERT(m_waitingVertices.empty(), "DGS fed non-DAG");
        }

        m_last = curIt;
        परत resultp;
    }

खाजगी:
    शून्य unblockDeps(स्थिरांक V3GraphVertex* vertexp) {
        जर (m_way == GraphWay::FORWARD) {
            साठी (स्थिरांक V3GraphEdge& edgep : vertexp->outEdges()) {
                स्थिरांक V3GraphVertex* स्थिरांक toVertexp = edgep.शीर्ष();

                स्थिरांक स्वतः it = m_waitingVertices.find(toVertexp);
                UASSERT_OBJ(it != m_waitingVertices.end(), toVertexp,
                            "Found edge into vertex not in waiting list.");
                जर (it->second.unblock()) {
                    m_readyVertices.घाला(it->second);
                    m_waitingVertices.erase(it);
                }
            }
        } अन्यथा {
            साठी (स्थिरांक V3GraphEdge& edge : vertexp->inEdges()) {
                स्थिरांक V3GraphVertex* स्थिरांक fromVertexp = edge.fromp();

                स्थिरांक स्वतः it = m_waitingVertices.find(fromVertexp);
                UASSERT_OBJ(it != m_waitingVertices.end(), fromVertexp,
                            "Found edge into vertex not in waiting list.");
                जर (it->second.unblock()) {
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
    स्थिरांक GraphWay m_way;  // Direction of traversal
    माप_प्रकार m_nextIndex = 0;  // Which index to return from m_nextVertices next
    std::vector<स्थिरांक V3GraphVertex*> m_nextVertices;  // List of ready vertices returned next
    std::vector<स्थिरांक V3GraphVertex*> m_readyVertices;  // List of other ready vertices

सार्वजनिक:
    // CONSTRUCTORS
    VL_UNCOPYABLE(GraphStreamUnordered);
    explicit GraphStreamUnordered(V3Graph* graphp, GraphWay way = GraphWay::FORWARD)
        : m_way{way} {
        जर (m_way == GraphWay::FORWARD) {
            init<GraphWay::FORWARD>(graphp);
        } अन्यथा {
            init<GraphWay::REVERSE>(graphp);
        }
    }
    ~GraphStreamUnordered() = पूर्वनिर्धारित;

    // METHODS

    // Each call to nextp() returns a unique vertex in the graph, in dependency order. Dependencies
    // alone do not specify a total ordering. Un-ordered vertices are returned in an arbitrary but
    // deterministic order.
    स्थिरांक V3GraphVertex* nextp() {
        जर (VL_UNLIKELY(m_nextIndex == m_nextVertices.size())) {
            जर (VL_UNLIKELY(m_readyVertices.empty())) परत nullptr;
            m_nextIndex = 0;
            // Use swap to avoid reallocation
            m_nextVertices.swap(m_readyVertices);
            m_readyVertices.clear();
        }
        स्थिरांक V3GraphVertex* स्थिरांक resultp = m_nextVertices[m_nextIndex++];
        जर (m_way == GraphWay::FORWARD) परत unblock<GraphWay::FORWARD>(resultp);
        परत unblock<GraphWay::REVERSE>(resultp);
    }

खाजगी:
    ढाँचा <uint8_t N_Way>  //
    VL_ATTR_NOINLINE शून्य init(V3Graph* graphp) {
        constexpr GraphWay way{N_Way};
        // Assign every vertex without an incoming edge to ready, others to waiting
        साठी (V3GraphVertex& vertex : graphp->vertices()) {
            स्थिरांक uint32_t nDeps = vertex.edges<way.invert()>().size();
            vertex.color(nDeps);  // Using color instead of user, as user might be used by client
            जर (VL_UNLIKELY(nDeps == 0)) m_nextVertices.ढकला_back(&vertex);
        }
    }

    ढाँचा <uint8_t N_Way>  //
    VL_ATTR_NOINLINE स्थिरांक V3GraphVertex* unblock(स्थिरांक V3GraphVertex* resultp) {
        constexpr GraphWay way{N_Way};
        साठी (स्थिरांक V3GraphEdge& edge : resultp->edges<way>()) {
            V3GraphVertex* स्थिरांक vertexp = edge.furtherp<way>();
#जर VL_DEBUG
            UASSERT_OBJ(vertexp->color() != 0, vertexp, "Should not be on waiting list");
#endif
            vertexp->color(vertexp->color() - 1);
            जर (!vertexp->color()) m_readyVertices.ढकला_back(vertexp);
        }
        परत resultp;  // Returning input so we can tail call this method
    }
};

#endif  // Guard
