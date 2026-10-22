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

#اگر_تعریف_نشده VERILATOR_V3GRAPHSTREAM_H_
#تعریف_کن VERILATOR_V3GRAPHSTREAM_H_

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

قالب <typename T_Compare>
class گراف_جریان final {
    // TYPES
    class VxHolder final {
    عام:
        // MEMBERS
        ثابت V3_گراف_رأس* ثابت m_vxp;  // [mtask] Vertex
        ثابت uint32_t m_pos;  // Sort position
        uint32_t m_رقم_Blocking_یالها;  // Number of blocking edges
        // CONSTRUCTORS
        VxHolder(ثابت V3_گراف_رأس* vxp, uint32_t pos, uint32_t رقم_Blocking_یالها)
            : m_vxp{vxp}
            , m_pos{pos}
            , m_رقم_Blocking_یالها{رقم_Blocking_یالها} {}
        // METHODS
        ثابت V3_گراف_رأس* vertexp() ثابت { أرجع m_vxp; }
        // Decrement blocking edges count, return true if the vertex is
        // newly unblocked
        bool unblock() {
            UASSERT_OBJ(m_رقم_Blocking_یالها > 0, vertexp(), "Underflow of blocking edges");
            --m_رقم_Blocking_یالها;
            أرجع (m_رقم_Blocking_یالها == 0);
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
        bool عملگر()(ثابت VxHolder& a, ثابت VxHolder& b) ثابت {
            إذا (m_lessThan.عملگر()(a.vertexp(), b.vertexp())) أرجع true;
            إذا (m_lessThan.عملگر()(b.vertexp(), a.vertexp())) أرجع false;
            أرجع a.m_pos < b.m_pos;
        }

    خاص:
        VL_UNCOPYABLE(VxHolderCmp);
    };

    using ReadyVertices = std::مجموعه<VxHolder, VxHolderCmp&>;

    // MEMBERS
    VxHolderCmp m_vxHolderCmp;  // Vertext comparison functor
    ReadyVertices m_readyVertices;  // List of ready vertices
    std::map<ثابت V3_گراف_رأس*, VxHolder> m_انتظار_Vertices;  // List of waiting vertices
    typename ReadyVertices::iterator m_آخرین;  // Previously returned element
    ثابت گراف_Way m_way;  // FORWARD or REVERSE order of traversal

عام:
    // CONSTRUCTORS
    explicit گراف_جریان(ثابت V3_گراف* graphp, گراف_Way way = گراف_Way::FORWARD,
                         ثابت T_Compare& lessThan = T_Compare())
        // NOTE: Perhaps REVERSE way should also reverse the sense of the
        // lessThan function? For now the only usage of REVERSE is not
        // sensitive to its lessThan at all, so it doesn't matter.
        : m_vxHolderCmp{lessThan}
        , m_readyVertices{m_vxHolderCmp}
        , m_آخرین{m_readyVertices.انتها()}
        , m_way{way} {
        uint32_t pos = 0;
        لكل (ثابت V3_گراف_رأس& vtx : graphp->vertices()) {
            // Every vertex initially is waiting, or ready.
            إذا (way == گراف_Way::FORWARD) {
                إذا (vtx.درون_خالی()) {
                    ثابت VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.درج(newVx);
                } وإلا {
                    ثابت uint32_t dep_شمار = vtx.درون_یالها().اندازه();
                    ثابت VxHolder newVx{&vtx, pos++, dep_شمار};
                    m_انتظار_Vertices.emplace(&vtx, newVx);
                }
            } وإلا {  //معکوس
                إذا (vtx.بیرون_خالی()) {
                    ثابت VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.درج(newVx);
                } وإلا {
                    ثابت uint32_t dep_شمار = vtx.بیرون_یالها().اندازه();
                    ثابت VxHolder newVx{&vtx, pos++, dep_شمار};
                    m_انتظار_Vertices.emplace(&vtx, newVx);
                }
            }
        }
    }
    ~گراف_جریان() = افتراضي;

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
    ثابت V3_گراف_رأس* nextp() {
        ثابت V3_گراف_رأس* resultp = nullptr;

        typename ReadyVertices::iterator curIt;
        إذا (m_آخرین == m_readyVertices.انتها()) {
            // First call to nextp()
            curIt = m_readyVertices.begin();
        } وإلا {
            // Subsequent call to nextp()
            curIt = m_آخرین;
            ++curIt;
            // Remove previously-returned element
            m_readyVertices.erase(m_آخرین);
            // Wrap curIt. Expect to wrap, and make another pass, to find
            // newly-ready elements that could have appeared ahead of the
            // m_last iterator
            إذا (curIt == m_readyVertices.انتها()) curIt = m_readyVertices.begin();
        }

        إذا (curIt != m_readyVertices.انتها()) {
            resultp = curIt->vertexp();
            unblockDeps(resultp);
        } وإلا {
            // No ready vertices; waiting should be empty too, otherwise we
            // were fed a graph with cycles (which is not supported.)
            UASSERT(m_انتظار_Vertices.خالی(), "DGS fed non-DAG");
        }

        m_آخرین = curIt;
        أرجع resultp;
    }

خاص:
    فراغ unblockDeps(ثابت V3_گراف_رأس* vertexp) {
        إذا (m_way == گراف_Way::FORWARD) {
            لكل (ثابت V3_گراف_یال& edgep : vertexp->بیرون_یالها()) {
                ثابت V3_گراف_رأس* ثابت مقصد_Vertexp = edgep.بالاترین();

                ثابت خودکار it = m_انتظار_Vertices.پیدا_کن(مقصد_Vertexp);
                UASSERT_OBJ(it != m_انتظار_Vertices.انتها(), مقصد_Vertexp,
                            "Found edge into vertex not in waiting list.");
                إذا (it->دومی.unblock()) {
                    m_readyVertices.درج(it->دومی);
                    m_انتظار_Vertices.erase(it);
                }
            }
        } وإلا {
            لكل (ثابت V3_گراف_یال& یال : vertexp->درون_یالها()) {
                ثابت V3_گراف_رأس* ثابت fromVertexp = یال.fromp();

                ثابت خودکار it = m_انتظار_Vertices.پیدا_کن(fromVertexp);
                UASSERT_OBJ(it != m_انتظار_Vertices.انتها(), fromVertexp,
                            "Found edge into vertex not in waiting list.");
                إذا (it->دومی.unblock()) {
                    m_readyVertices.درج(it->دومی);
                    m_انتظار_Vertices.erase(it);
                }
            }
        }
    }

    VL_UNCOPYABLE(گراف_جریان);
};

//=================================================================================================
// GraphStreamUnordered is similar to GraphStream, but iterates un-ordered vertices (those that are
// not ordered by dependencies) in an arbitrary order. Iteration order is still deterministic.

class گراف_جریان_Unordered final {
    // MEMBERS
    ثابت گراف_Way m_way;  // Direction of traversal
    اندازه_t m_بعدی_شاخص = 0;  // Which index to return from m_nextVertices next
    std::وکتور<ثابت V3_گراف_رأس*> m_بعدی_Vertices;  // List of ready vertices returned next
    std::وکتور<ثابت V3_گراف_رأس*> m_readyVertices;  // List of other ready vertices

عام:
    // CONSTRUCTORS
    VL_UNCOPYABLE(گراف_جریان_Unordered);
    explicit گراف_جریان_Unordered(V3_گراف* graphp, گراف_Way way = گراف_Way::FORWARD)
        : m_way{way} {
        إذا (m_way == گراف_Way::FORWARD) {
            init<گراف_Way::FORWARD>(graphp);
        } وإلا {
            init<گراف_Way::معکوس_2>(graphp);
        }
    }
    ~گراف_جریان_Unordered() = افتراضي;

    // METHODS

    // Each call to nextp() returns a unique vertex in the graph, in dependency order. Dependencies
    // alone do not specify a total ordering. Un-ordered vertices are returned in an arbitrary but
    // deterministic order.
    ثابت V3_گراف_رأس* nextp() {
        إذا (VL_UNLIKELY(m_بعدی_شاخص == m_بعدی_Vertices.اندازه())) {
            إذا (VL_UNLIKELY(m_readyVertices.خالی())) أرجع nullptr;
            m_بعدی_شاخص = 0;
            // Use swap to avoid reallocation
            m_بعدی_Vertices.جابجا(m_readyVertices);
            m_readyVertices.clear();
        }
        ثابت V3_گراف_رأس* ثابت resultp = m_بعدی_Vertices[m_بعدی_شاخص++];
        إذا (m_way == گراف_Way::FORWARD) أرجع unblock<گراف_Way::FORWARD>(resultp);
        أرجع unblock<گراف_Way::معکوس_2>(resultp);
    }

خاص:
    قالب <uint8_t N_Way>  //
    VL_ATTR_NOINLINE فراغ init(V3_گراف* graphp) {
        constexpr گراف_Way way{N_Way};
        // Assign every vertex without an incoming edge to ready, others to waiting
        لكل (V3_گراف_رأس& رأس : graphp->vertices()) {
            ثابت uint32_t nDeps = رأس.یالها<way.invert()>().اندازه();
            رأس.color(nDeps);  // Using color instead of user, as user might be used by client
            إذا (VL_UNLIKELY(nDeps == 0)) m_بعدی_Vertices.افزودن_پشت(&رأس);
        }
    }

    قالب <uint8_t N_Way>  //
    VL_ATTR_NOINLINE ثابت V3_گراف_رأس* unblock(ثابت V3_گراف_رأس* resultp) {
        constexpr گراف_Way way{N_Way};
        لكل (ثابت V3_گراف_یال& یال : resultp->یالها<way>()) {
            V3_گراف_رأس* ثابت vertexp = یال.furtherp<way>();
#إذا VL_DEBUG
            UASSERT_OBJ(vertexp->color() != 0, vertexp, "Should not be on waiting list");
#پایان_اگر
            vertexp->color(vertexp->color() - 1);
            إذا (!vertexp->color()) m_readyVertices.افزودن_پشت(vertexp);
        }
        أرجع resultp;  // Returning input so we can tail call this method
    }
};

#پایان_اگر  // Guard
