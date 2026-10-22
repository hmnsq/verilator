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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3GRAPHSTREAM_H_
#وصف_ڪريو VERILATOR_V3GRAPHSTREAM_H_

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

سانچو <typename T_Compare>
class گراف_وهڪرو final {
    // TYPES
    class VxHolder final {
    عام:
        // MEMBERS
        ثابت V3_گراف_راس* ثابت m_vxp;  // [mtask] Vertex
        ثابت uint32_t m_pos;  // Sort position
        uint32_t m_انگ_Blocking_ڪنارا;  // Number of blocking edges
        // CONSTRUCTORS
        VxHolder(ثابت V3_گراف_راس* vxp, uint32_t pos, uint32_t انگ_Blocking_ڪنارا)
            : m_vxp{vxp}
            , m_pos{pos}
            , m_انگ_Blocking_ڪنارا{انگ_Blocking_ڪنارا} {}
        // METHODS
        ثابت V3_گراف_راس* vertexp() ثابت { أرجع m_vxp; }
        // Decrement blocking edges count, return true if the vertex is
        // newly unblocked
        bool unblock() {
            UASSERT_OBJ(m_انگ_Blocking_ڪنارا > 0, vertexp(), "Underflow of blocking edges");
            --m_انگ_Blocking_ڪنارا;
            أرجع (m_انگ_Blocking_ڪنارا == 0);
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
        bool عامل()(ثابت VxHolder& a, ثابت VxHolder& b) ثابت {
            إذا (m_lessThan.عامل()(a.vertexp(), b.vertexp())) أرجع true;
            إذا (m_lessThan.عامل()(b.vertexp(), a.vertexp())) أرجع false;
            أرجع a.m_pos < b.m_pos;
        }

    خاص:
        VL_UNCOPYABLE(VxHolderCmp);
    };

    using ReadyVertices = std::سيٽ<VxHolder, VxHolderCmp&>;

    // MEMBERS
    VxHolderCmp m_vxHolderCmp;  // Vertext comparison functor
    ReadyVertices m_readyVertices;  // List of ready vertices
    std::map<ثابت V3_گراف_راس*, VxHolder> m_انتظار_Vertices;  // List of waiting vertices
    typename ReadyVertices::iterator m_آخري;  // Previously returned element
    ثابت گراف_Way m_way;  // FORWARD or REVERSE order of traversal

عام:
    // CONSTRUCTORS
    explicit گراف_وهڪرو(ثابت V3_گراف* graphp, گراف_Way way = گراف_Way::FORWARD,
                         ثابت T_Compare& lessThan = T_Compare())
        // NOTE: Perhaps REVERSE way should also reverse the sense of the
        // lessThan function? For now the only usage of REVERSE is not
        // sensitive to its lessThan at all, so it doesn't matter.
        : m_vxHolderCmp{lessThan}
        , m_readyVertices{m_vxHolderCmp}
        , m_آخري{m_readyVertices.پڇاڙي()}
        , m_way{way} {
        uint32_t pos = 0;
        لكل (ثابت V3_گراف_راس& vtx : graphp->vertices()) {
            // Every vertex initially is waiting, or ready.
            إذا (way == گراف_Way::FORWARD) {
                إذا (vtx.اندريون_خالي_جايون()) {
                    ثابت VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.وجهو(newVx);
                } وإلا {
                    ثابت uint32_t dep_ڳڻپ = vtx.اندريون_ڪنارا().سائيز();
                    ثابت VxHolder newVx{&vtx, pos++, dep_ڳڻپ};
                    m_انتظار_Vertices.emplace(&vtx, newVx);
                }
            } وإلا {  //ابتو
                إذا (vtx.ٻاهريون_خالي_جايون()) {
                    ثابت VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.وجهو(newVx);
                } وإلا {
                    ثابت uint32_t dep_ڳڻپ = vtx.ٻاهريون_ڪنارا().سائيز();
                    ثابت VxHolder newVx{&vtx, pos++, dep_ڳڻپ};
                    m_انتظار_Vertices.emplace(&vtx, newVx);
                }
            }
        }
    }
    ~گراف_وهڪرو() = افتراضي;

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
    ثابت V3_گراف_راس* nextp() {
        ثابت V3_گراف_راس* resultp = nullptr;

        typename ReadyVertices::iterator curIt;
        إذا (m_آخري == m_readyVertices.پڇاڙي()) {
            // First call to nextp()
            curIt = m_readyVertices.begin();
        } وإلا {
            // Subsequent call to nextp()
            curIt = m_آخري;
            ++curIt;
            // Remove previously-returned element
            m_readyVertices.erase(m_آخري);
            // Wrap curIt. Expect to wrap, and make another pass, to find
            // newly-ready elements that could have appeared ahead of the
            // m_last iterator
            إذا (curIt == m_readyVertices.پڇاڙي()) curIt = m_readyVertices.begin();
        }

        إذا (curIt != m_readyVertices.پڇاڙي()) {
            resultp = curIt->vertexp();
            unblockDeps(resultp);
        } وإلا {
            // No ready vertices; waiting should be empty too, otherwise we
            // were fed a graph with cycles (which is not supported.)
            UASSERT(m_انتظار_Vertices.خالي_جايون(), "DGS fed non-DAG");
        }

        m_آخري = curIt;
        أرجع resultp;
    }

خاص:
    فراغ unblockDeps(ثابت V3_گراف_راس* vertexp) {
        إذا (m_way == گراف_Way::FORWARD) {
            لكل (ثابت V3_گراف_ڪنارو& edgep : vertexp->ٻاهريون_ڪنارا()) {
                ثابت V3_گراف_راس* ثابت منزل_Vertexp = edgep.چوٽي();

                ثابت خودڪار it = m_انتظار_Vertices.ڳوليو(منزل_Vertexp);
                UASSERT_OBJ(it != m_انتظار_Vertices.پڇاڙي(), منزل_Vertexp,
                            "Found edge into vertex not in waiting list.");
                إذا (it->ٻيو.unblock()) {
                    m_readyVertices.وجهو(it->ٻيو);
                    m_انتظار_Vertices.erase(it);
                }
            }
        } وإلا {
            لكل (ثابت V3_گراف_ڪنارو& ڪنارو : vertexp->اندريون_ڪنارا()) {
                ثابت V3_گراف_راس* ثابت fromVertexp = ڪنارو.fromp();

                ثابت خودڪار it = m_انتظار_Vertices.ڳوليو(fromVertexp);
                UASSERT_OBJ(it != m_انتظار_Vertices.پڇاڙي(), fromVertexp,
                            "Found edge into vertex not in waiting list.");
                إذا (it->ٻيو.unblock()) {
                    m_readyVertices.وجهو(it->ٻيو);
                    m_انتظار_Vertices.erase(it);
                }
            }
        }
    }

    VL_UNCOPYABLE(گراف_وهڪرو);
};

//=================================================================================================
// GraphStreamUnordered is similar to GraphStream, but iterates un-ordered vertices (those that are
// not ordered by dependencies) in an arbitrary order. Iteration order is still deterministic.

class گراف_وهڪرو_Unordered final {
    // MEMBERS
    ثابت گراف_Way m_way;  // Direction of traversal
    سائيز_t m_ايندڙ_اشاريو = 0;  // Which index to return from m_nextVertices next
    std::ويڪٽر<ثابت V3_گراف_راس*> m_ايندڙ_Vertices;  // List of ready vertices returned next
    std::ويڪٽر<ثابت V3_گراف_راس*> m_readyVertices;  // List of other ready vertices

عام:
    // CONSTRUCTORS
    VL_UNCOPYABLE(گراف_وهڪرو_Unordered);
    explicit گراف_وهڪرو_Unordered(V3_گراف* graphp, گراف_Way way = گراف_Way::FORWARD)
        : m_way{way} {
        إذا (m_way == گراف_Way::FORWARD) {
            init<گراف_Way::FORWARD>(graphp);
        } وإلا {
            init<گراف_Way::ابتو_2>(graphp);
        }
    }
    ~گراف_وهڪرو_Unordered() = افتراضي;

    // METHODS

    // Each call to nextp() returns a unique vertex in the graph, in dependency order. Dependencies
    // alone do not specify a total ordering. Un-ordered vertices are returned in an arbitrary but
    // deterministic order.
    ثابت V3_گراف_راس* nextp() {
        إذا (VL_UNLIKELY(m_ايندڙ_اشاريو == m_ايندڙ_Vertices.سائيز())) {
            إذا (VL_UNLIKELY(m_readyVertices.خالي_جايون())) أرجع nullptr;
            m_ايندڙ_اشاريو = 0;
            // Use swap to avoid reallocation
            m_ايندڙ_Vertices.مٽا_سٽا(m_readyVertices);
            m_readyVertices.clear();
        }
        ثابت V3_گراف_راس* ثابت resultp = m_ايندڙ_Vertices[m_ايندڙ_اشاريو++];
        إذا (m_way == گراف_Way::FORWARD) أرجع unblock<گراف_Way::FORWARD>(resultp);
        أرجع unblock<گراف_Way::ابتو_2>(resultp);
    }

خاص:
    سانچو <uint8_t N_Way>  //
    VL_ATTR_NOINLINE فراغ init(V3_گراف* graphp) {
        constexpr گراف_Way way{N_Way};
        // Assign every vertex without an incoming edge to ready, others to waiting
        لكل (V3_گراف_راس& راس : graphp->vertices()) {
            ثابت uint32_t nDeps = راس.ڪنارا<way.invert()>().سائيز();
            راس.color(nDeps);  // Using color instead of user, as user might be used by client
            إذا (VL_UNLIKELY(nDeps == 0)) m_ايندڙ_Vertices.ڌڪو_پٺ(&راس);
        }
    }

    سانچو <uint8_t N_Way>  //
    VL_ATTR_NOINLINE ثابت V3_گراف_راس* unblock(ثابت V3_گراف_راس* resultp) {
        constexpr گراف_Way way{N_Way};
        لكل (ثابت V3_گراف_ڪنارو& ڪنارو : resultp->ڪنارا<way>()) {
            V3_گراف_راس* ثابت vertexp = ڪنارو.furtherp<way>();
#إذا VL_DEBUG
            UASSERT_OBJ(vertexp->color() != 0, vertexp, "Should not be on waiting list");
#جيڪڏهن_پورو
            vertexp->color(vertexp->color() - 1);
            إذا (!vertexp->color()) m_readyVertices.ڌڪو_پٺ(vertexp);
        }
        أرجع resultp;  // Returning input so we can tail call this method
    }
};

#جيڪڏهن_پورو  // Guard
