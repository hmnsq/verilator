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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3GRAPHSTREAM_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3GRAPHSTREAM_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <functional>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

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

ਸਾਂਚਾ <typename T_Compare>
class ਗ੍ਰਾਫ_ਧਾਰਾ final {
    // TYPES
    class VxHolder final {
    ਜਨਤਕ:
        // MEMBERS
        ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ m_vxp;  // [mtask] Vertex
        ਅਚਲ uint32_t m_pos;  // Sort position
        uint32_t m_ਅੰਕ_Blocking_ਕਿਨਾਰੇ;  // Number of blocking edges
        // CONSTRUCTORS
        VxHolder(ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vxp, uint32_t pos, uint32_t ਅੰਕ_Blocking_ਕਿਨਾਰੇ)
            : m_vxp{vxp}
            , m_pos{pos}
            , m_ਅੰਕ_Blocking_ਕਿਨਾਰੇ{ਅੰਕ_Blocking_ਕਿਨਾਰੇ} {}
        // METHODS
        ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp() ਅਚਲ { ਵਾਪਸ m_vxp; }
        // Decrement blocking edges count, return true if the vertex is
        // newly unblocked
        bool unblock() {
            UASSERT_OBJ(m_ਅੰਕ_Blocking_ਕਿਨਾਰੇ > 0, vertexp(), "Underflow of blocking edges");
            --m_ਅੰਕ_Blocking_ਕਿਨਾਰੇ;
            ਵਾਪਸ (m_ਅੰਕ_Blocking_ਕਿਨਾਰੇ == 0);
        }
    };

    class VxHolderCmp final {
    ਜਨਤਕ:
        // MEMBERS
        ਅਚਲ T_Compare m_lessThan;  // Sorting functor
        // CONSTRUCTORS
        explicit VxHolderCmp(ਅਚਲ T_Compare& lessThan)
            : m_lessThan{lessThan} {}
        // METHODS
        bool ਸੰਚਾਲਕ()(ਅਚਲ VxHolder& a, ਅਚਲ VxHolder& b) ਅਚਲ {
            ਜੇ (m_lessThan.ਸੰਚਾਲਕ()(a.vertexp(), b.vertexp())) ਵਾਪਸ true;
            ਜੇ (m_lessThan.ਸੰਚਾਲਕ()(b.vertexp(), a.vertexp())) ਵਾਪਸ false;
            ਵਾਪਸ a.m_pos < b.m_pos;
        }

    ਨਿਜੀ:
        VL_UNCOPYABLE(VxHolderCmp);
    };

    using ReadyVertices = std::ਸੈੱਟ<VxHolder, VxHolderCmp&>;

    // MEMBERS
    VxHolderCmp m_vxHolderCmp;  // Vertext comparison functor
    ReadyVertices m_readyVertices;  // List of ready vertices
    std::map<ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*, VxHolder> m_ਉਡੀਕ_Vertices;  // List of waiting vertices
    typename ReadyVertices::iterator m_ਆਖਰੀ;  // Previously returned element
    ਅਚਲ ਗ੍ਰਾਫ_Way m_way;  // FORWARD or REVERSE order of traversal

ਜਨਤਕ:
    // CONSTRUCTORS
    explicit ਗ੍ਰਾਫ_ਧਾਰਾ(ਅਚਲ V3_ਗ੍ਰਾਫ* graphp, ਗ੍ਰਾਫ_Way way = ਗ੍ਰਾਫ_Way::FORWARD,
                         ਅਚਲ T_Compare& lessThan = T_Compare())
        // NOTE: Perhaps REVERSE way should also reverse the sense of the
        // lessThan function? For now the only usage of REVERSE is not
        // sensitive to its lessThan at all, so it doesn't matter.
        : m_vxHolderCmp{lessThan}
        , m_readyVertices{m_vxHolderCmp}
        , m_ਆਖਰੀ{m_readyVertices.ਸਿਰਾ()}
        , m_way{way} {
        uint32_t pos = 0;
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : graphp->vertices()) {
            // Every vertex initially is waiting, or ready.
            ਜੇ (way == ਗ੍ਰਾਫ_Way::FORWARD) {
                ਜੇ (vtx.ਵਿੱਚ_ਖਾਲੀ_ਥਾਵਾਂ()) {
                    ਅਚਲ VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.ਪਾਓ(newVx);
                } ਨਹੀਂਤਾਂ {
                    ਅਚਲ uint32_t dep_ਗਿਣਤੀ = vtx.ਵਿੱਚ_ਕਿਨਾਰੇ().ਮਾਪ();
                    ਅਚਲ VxHolder newVx{&vtx, pos++, dep_ਗਿਣਤੀ};
                    m_ਉਡੀਕ_Vertices.emplace(&vtx, newVx);
                }
            } ਨਹੀਂਤਾਂ {  //ਉਲਟ
                ਜੇ (vtx.ਬਾਹਰਲਾ_ਖਾਲੀ_ਥਾਵਾਂ()) {
                    ਅਚਲ VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.ਪਾਓ(newVx);
                } ਨਹੀਂਤਾਂ {
                    ਅਚਲ uint32_t dep_ਗਿਣਤੀ = vtx.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().ਮਾਪ();
                    ਅਚਲ VxHolder newVx{&vtx, pos++, dep_ਗਿਣਤੀ};
                    m_ਉਡੀਕ_Vertices.emplace(&vtx, newVx);
                }
            }
        }
    }
    ~ਗ੍ਰਾਫ_ਧਾਰਾ() = ਮੂਲ;

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
    ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* nextp() {
        ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* resultp = nullptr;

        typename ReadyVertices::iterator curIt;
        ਜੇ (m_ਆਖਰੀ == m_readyVertices.ਸਿਰਾ()) {
            // First call to nextp()
            curIt = m_readyVertices.begin();
        } ਨਹੀਂਤਾਂ {
            // Subsequent call to nextp()
            curIt = m_ਆਖਰੀ;
            ++curIt;
            // Remove previously-returned element
            m_readyVertices.erase(m_ਆਖਰੀ);
            // Wrap curIt. Expect to wrap, and make another pass, to find
            // newly-ready elements that could have appeared ahead of the
            // m_last iterator
            ਜੇ (curIt == m_readyVertices.ਸਿਰਾ()) curIt = m_readyVertices.begin();
        }

        ਜੇ (curIt != m_readyVertices.ਸਿਰਾ()) {
            resultp = curIt->vertexp();
            unblockDeps(resultp);
        } ਨਹੀਂਤਾਂ {
            // No ready vertices; waiting should be empty too, otherwise we
            // were fed a graph with cycles (which is not supported.)
            UASSERT(m_ਉਡੀਕ_Vertices.ਖਾਲੀ_ਥਾਵਾਂ(), "DGS fed non-DAG");
        }

        m_ਆਖਰੀ = curIt;
        ਵਾਪਸ resultp;
    }

ਨਿਜੀ:
    ਖਾਲੀ unblockDeps(ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp) {
        ਜੇ (m_way == ਗ੍ਰਾਫ_Way::FORWARD) {
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& edgep : vertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ ਮੰਜ਼ਿਲ_Vertexp = edgep.ਸਿਖਰ();

                ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_ਉਡੀਕ_Vertices.ਲੱਭੋ(ਮੰਜ਼ਿਲ_Vertexp);
                UASSERT_OBJ(it != m_ਉਡੀਕ_Vertices.ਸਿਰਾ(), ਮੰਜ਼ਿਲ_Vertexp,
                            "Found edge into vertex not in waiting list.");
                ਜੇ (it->ਦੂਜਾ.unblock()) {
                    m_readyVertices.ਪਾਓ(it->ਦੂਜਾ);
                    m_ਉਡੀਕ_Vertices.erase(it);
                }
            }
        } ਨਹੀਂਤਾਂ {
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vertexp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
                ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ fromVertexp = ਕਿਨਾਰਾ.fromp();

                ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_ਉਡੀਕ_Vertices.ਲੱਭੋ(fromVertexp);
                UASSERT_OBJ(it != m_ਉਡੀਕ_Vertices.ਸਿਰਾ(), fromVertexp,
                            "Found edge into vertex not in waiting list.");
                ਜੇ (it->ਦੂਜਾ.unblock()) {
                    m_readyVertices.ਪਾਓ(it->ਦੂਜਾ);
                    m_ਉਡੀਕ_Vertices.erase(it);
                }
            }
        }
    }

    VL_UNCOPYABLE(ਗ੍ਰਾਫ_ਧਾਰਾ);
};

//=================================================================================================
// GraphStreamUnordered is similar to GraphStream, but iterates un-ordered vertices (those that are
// not ordered by dependencies) in an arbitrary order. Iteration order is still deterministic.

class ਗ੍ਰਾਫ_ਧਾਰਾ_Unordered final {
    // MEMBERS
    ਅਚਲ ਗ੍ਰਾਫ_Way m_way;  // Direction of traversal
    ਮਾਪ_t m_ਅਗਲਾ_ਸੂਚਕਾਂਕ = 0;  // Which index to return from m_nextVertices next
    std::ਸਦਿਸ਼<ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*> m_ਅਗਲਾ_Vertices;  // List of ready vertices returned next
    std::ਸਦਿਸ਼<ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*> m_readyVertices;  // List of other ready vertices

ਜਨਤਕ:
    // CONSTRUCTORS
    VL_UNCOPYABLE(ਗ੍ਰਾਫ_ਧਾਰਾ_Unordered);
    explicit ਗ੍ਰਾਫ_ਧਾਰਾ_Unordered(V3_ਗ੍ਰਾਫ* graphp, ਗ੍ਰਾਫ_Way way = ਗ੍ਰਾਫ_Way::FORWARD)
        : m_way{way} {
        ਜੇ (m_way == ਗ੍ਰਾਫ_Way::FORWARD) {
            init<ਗ੍ਰਾਫ_Way::FORWARD>(graphp);
        } ਨਹੀਂਤਾਂ {
            init<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>(graphp);
        }
    }
    ~ਗ੍ਰਾਫ_ਧਾਰਾ_Unordered() = ਮੂਲ;

    // METHODS

    // Each call to nextp() returns a unique vertex in the graph, in dependency order. Dependencies
    // alone do not specify a total ordering. Un-ordered vertices are returned in an arbitrary but
    // deterministic order.
    ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* nextp() {
        ਜੇ (VL_UNLIKELY(m_ਅਗਲਾ_ਸੂਚਕਾਂਕ == m_ਅਗਲਾ_Vertices.ਮਾਪ())) {
            ਜੇ (VL_UNLIKELY(m_readyVertices.ਖਾਲੀ_ਥਾਵਾਂ())) ਵਾਪਸ nullptr;
            m_ਅਗਲਾ_ਸੂਚਕਾਂਕ = 0;
            // Use swap to avoid reallocation
            m_ਅਗਲਾ_Vertices.ਅਦਲਾ_ਬਦਲੀ(m_readyVertices);
            m_readyVertices.clear();
        }
        ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ resultp = m_ਅਗਲਾ_Vertices[m_ਅਗਲਾ_ਸੂਚਕਾਂਕ++];
        ਜੇ (m_way == ਗ੍ਰਾਫ_Way::FORWARD) ਵਾਪਸ unblock<ਗ੍ਰਾਫ_Way::FORWARD>(resultp);
        ਵਾਪਸ unblock<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>(resultp);
    }

ਨਿਜੀ:
    ਸਾਂਚਾ <uint8_t N_Way>  //
    VL_ATTR_NOINLINE ਖਾਲੀ init(V3_ਗ੍ਰਾਫ* graphp) {
        constexpr ਗ੍ਰਾਫ_Way way{N_Way};
        // Assign every vertex without an incoming edge to ready, others to waiting
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : graphp->vertices()) {
            ਅਚਲ uint32_t nDeps = ਸ਼ੀਰਸ਼.ਕਿਨਾਰੇ<way.invert()>().ਮਾਪ();
            ਸ਼ੀਰਸ਼.color(nDeps);  // Using color instead of user, as user might be used by client
            ਜੇ (VL_UNLIKELY(nDeps == 0)) m_ਅਗਲਾ_Vertices.ਧੱਕੋ_ਪਿੱਠ(&ਸ਼ੀਰਸ਼);
        }
    }

    ਸਾਂਚਾ <uint8_t N_Way>  //
    VL_ATTR_NOINLINE ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* unblock(ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* resultp) {
        constexpr ਗ੍ਰਾਫ_Way way{N_Way};
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : resultp->ਕਿਨਾਰੇ<way>()) {
            V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vertexp = ਕਿਨਾਰਾ.furtherp<way>();
#ਜੇ VL_DEBUG
            UASSERT_OBJ(vertexp->color() != 0, vertexp, "Should not be on waiting list");
#ਜੇ_ਅੰਤ
            vertexp->color(vertexp->color() - 1);
            ਜੇ (!vertexp->color()) m_readyVertices.ਧੱਕੋ_ਪਿੱਠ(vertexp);
        }
        ਵਾਪਸ resultp;  // Returning input so we can tail call this method
    }
};

#ਜੇ_ਅੰਤ  // Guard
