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

#که_نه_تعریف_شوی VERILATOR_V3GRAPHSTREAM_H_
#تعریف_کړه VERILATOR_V3GRAPHSTREAM_H_

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

کينډۍ <typename T_Compare>
class ګراف_بهیر final {
    // TYPES
    class VxHolder final {
    عام:
        // MEMBERS
        ثابت V3_ګراف_راس* ثابت m_vxp;  // [mtask] Vertex
        ثابت uint32_t m_pos;  // Sort position
        uint32_t m_عدد_Blocking_څنډې;  // Number of blocking edges
        // CONSTRUCTORS
        VxHolder(ثابت V3_ګراف_راس* vxp, uint32_t pos, uint32_t عدد_Blocking_څنډې)
            : m_vxp{vxp}
            , m_pos{pos}
            , m_عدد_Blocking_څنډې{عدد_Blocking_څنډې} {}
        // METHODS
        ثابت V3_ګراف_راس* vertexp() ثابت { أرجع m_vxp; }
        // Decrement blocking edges count, return true if the vertex is
        // newly unblocked
        bool unblock() {
            UASSERT_OBJ(m_عدد_Blocking_څنډې > 0, vertexp(), "Underflow of blocking edges");
            --m_عدد_Blocking_څنډې;
            أرجع (m_عدد_Blocking_څنډې == 0);
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
        bool چلوونکی()(ثابت VxHolder& a, ثابت VxHolder& b) ثابت {
            إذا (m_lessThan.چلوونکی()(a.vertexp(), b.vertexp())) أرجع true;
            إذا (m_lessThan.چلوونکی()(b.vertexp(), a.vertexp())) أرجع false;
            أرجع a.m_pos < b.m_pos;
        }

    خاص:
        VL_UNCOPYABLE(VxHolderCmp);
    };

    using ReadyVertices = std::سېټ<VxHolder, VxHolderCmp&>;

    // MEMBERS
    VxHolderCmp m_vxHolderCmp;  // Vertext comparison functor
    ReadyVertices m_readyVertices;  // List of ready vertices
    std::map<ثابت V3_ګراف_راس*, VxHolder> m_انتظار_Vertices;  // List of waiting vertices
    typename ReadyVertices::iterator m_وروستی;  // Previously returned element
    ثابت ګراف_Way m_way;  // FORWARD or REVERSE order of traversal

عام:
    // CONSTRUCTORS
    explicit ګراف_بهیر(ثابت V3_ګراف* graphp, ګراف_Way way = ګراف_Way::FORWARD,
                         ثابت T_Compare& lessThan = T_Compare())
        // NOTE: Perhaps REVERSE way should also reverse the sense of the
        // lessThan function? For now the only usage of REVERSE is not
        // sensitive to its lessThan at all, so it doesn't matter.
        : m_vxHolderCmp{lessThan}
        , m_readyVertices{m_vxHolderCmp}
        , m_وروستی{m_readyVertices.پای_ټکی()}
        , m_way{way} {
        uint32_t pos = 0;
        لكل (ثابت V3_ګراف_راس& vtx : graphp->vertices()) {
            // Every vertex initially is waiting, or ready.
            إذا (way == ګراف_Way::FORWARD) {
                إذا (vtx.دننه_تشه()) {
                    ثابت VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.ورننباسه(newVx);
                } وإلا {
                    ثابت uint32_t dep_شمېر = vtx.دننه_څنډې().اندازه();
                    ثابت VxHolder newVx{&vtx, pos++, dep_شمېر};
                    m_انتظار_Vertices.emplace(&vtx, newVx);
                }
            } وإلا {  //سرچپه
                إذا (vtx.بهر_تشه()) {
                    ثابت VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.ورننباسه(newVx);
                } وإلا {
                    ثابت uint32_t dep_شمېر = vtx.بهر_څنډې().اندازه();
                    ثابت VxHolder newVx{&vtx, pos++, dep_شمېر};
                    m_انتظار_Vertices.emplace(&vtx, newVx);
                }
            }
        }
    }
    ~ګراف_بهیر() = افتراضي;

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
    ثابت V3_ګراف_راس* nextp() {
        ثابت V3_ګراف_راس* resultp = nullptr;

        typename ReadyVertices::iterator curIt;
        إذا (m_وروستی == m_readyVertices.پای_ټکی()) {
            // First call to nextp()
            curIt = m_readyVertices.begin();
        } وإلا {
            // Subsequent call to nextp()
            curIt = m_وروستی;
            ++curIt;
            // Remove previously-returned element
            m_readyVertices.erase(m_وروستی);
            // Wrap curIt. Expect to wrap, and make another pass, to find
            // newly-ready elements that could have appeared ahead of the
            // m_last iterator
            إذا (curIt == m_readyVertices.پای_ټکی()) curIt = m_readyVertices.begin();
        }

        إذا (curIt != m_readyVertices.پای_ټکی()) {
            resultp = curIt->vertexp();
            unblockDeps(resultp);
        } وإلا {
            // No ready vertices; waiting should be empty too, otherwise we
            // were fed a graph with cycles (which is not supported.)
            UASSERT(m_انتظار_Vertices.تشه(), "DGS fed non-DAG");
        }

        m_وروستی = curIt;
        أرجع resultp;
    }

خاص:
    فراغ unblockDeps(ثابت V3_ګراف_راس* vertexp) {
        إذا (m_way == ګراف_Way::FORWARD) {
            لكل (ثابت V3_ګراف_څنډه& edgep : vertexp->بهر_څنډې()) {
                ثابت V3_ګراف_راس* ثابت منزل_Vertexp = edgep.سر();

                ثابت خپلکاری it = m_انتظار_Vertices.پیدا_کړه(منزل_Vertexp);
                UASSERT_OBJ(it != m_انتظار_Vertices.پای_ټکی(), منزل_Vertexp,
                            "Found edge into vertex not in waiting list.");
                إذا (it->دوهم.unblock()) {
                    m_readyVertices.ورننباسه(it->دوهم);
                    m_انتظار_Vertices.erase(it);
                }
            }
        } وإلا {
            لكل (ثابت V3_ګراف_څنډه& څنډه : vertexp->دننه_څنډې()) {
                ثابت V3_ګراف_راس* ثابت fromVertexp = څنډه.fromp();

                ثابت خپلکاری it = m_انتظار_Vertices.پیدا_کړه(fromVertexp);
                UASSERT_OBJ(it != m_انتظار_Vertices.پای_ټکی(), fromVertexp,
                            "Found edge into vertex not in waiting list.");
                إذا (it->دوهم.unblock()) {
                    m_readyVertices.ورننباسه(it->دوهم);
                    m_انتظار_Vertices.erase(it);
                }
            }
        }
    }

    VL_UNCOPYABLE(ګراف_بهیر);
};

//=================================================================================================
// GraphStreamUnordered is similar to GraphStream, but iterates un-ordered vertices (those that are
// not ordered by dependencies) in an arbitrary order. Iteration order is still deterministic.

class ګراف_بهیر_Unordered final {
    // MEMBERS
    ثابت ګراف_Way m_way;  // Direction of traversal
    اندازه_t m_بل_شاخص = 0;  // Which index to return from m_nextVertices next
    std::ویکټور<ثابت V3_ګراف_راس*> m_بل_Vertices;  // List of ready vertices returned next
    std::ویکټور<ثابت V3_ګراف_راس*> m_readyVertices;  // List of other ready vertices

عام:
    // CONSTRUCTORS
    VL_UNCOPYABLE(ګراف_بهیر_Unordered);
    explicit ګراف_بهیر_Unordered(V3_ګراف* graphp, ګراف_Way way = ګراف_Way::FORWARD)
        : m_way{way} {
        إذا (m_way == ګراف_Way::FORWARD) {
            init<ګراف_Way::FORWARD>(graphp);
        } وإلا {
            init<ګراف_Way::سرچپه_2>(graphp);
        }
    }
    ~ګراف_بهیر_Unordered() = افتراضي;

    // METHODS

    // Each call to nextp() returns a unique vertex in the graph, in dependency order. Dependencies
    // alone do not specify a total ordering. Un-ordered vertices are returned in an arbitrary but
    // deterministic order.
    ثابت V3_ګراف_راس* nextp() {
        إذا (VL_UNLIKELY(m_بل_شاخص == m_بل_Vertices.اندازه())) {
            إذا (VL_UNLIKELY(m_readyVertices.تشه())) أرجع nullptr;
            m_بل_شاخص = 0;
            // Use swap to avoid reallocation
            m_بل_Vertices.بدلون(m_readyVertices);
            m_readyVertices.clear();
        }
        ثابت V3_ګراف_راس* ثابت resultp = m_بل_Vertices[m_بل_شاخص++];
        إذا (m_way == ګراف_Way::FORWARD) أرجع unblock<ګراف_Way::FORWARD>(resultp);
        أرجع unblock<ګراف_Way::سرچپه_2>(resultp);
    }

خاص:
    کينډۍ <uint8_t N_Way>  //
    VL_ATTR_NOINLINE فراغ init(V3_ګراف* graphp) {
        constexpr ګراف_Way way{N_Way};
        // Assign every vertex without an incoming edge to ready, others to waiting
        لكل (V3_ګراف_راس& راس : graphp->vertices()) {
            ثابت uint32_t nDeps = راس.څنډې<way.invert()>().اندازه();
            راس.color(nDeps);  // Using color instead of user, as user might be used by client
            إذا (VL_UNLIKELY(nDeps == 0)) m_بل_Vertices.ټېل_شا(&راس);
        }
    }

    کينډۍ <uint8_t N_Way>  //
    VL_ATTR_NOINLINE ثابت V3_ګراف_راس* unblock(ثابت V3_ګراف_راس* resultp) {
        constexpr ګراف_Way way{N_Way};
        لكل (ثابت V3_ګراف_څنډه& څنډه : resultp->څنډې<way>()) {
            V3_ګراف_راس* ثابت vertexp = څنډه.furtherp<way>();
#إذا VL_DEBUG
            UASSERT_OBJ(vertexp->color() != 0, vertexp, "Should not be on waiting list");
#د_که_پای
            vertexp->color(vertexp->color() - 1);
            إذا (!vertexp->color()) m_readyVertices.ټېل_شا(vertexp);
        }
        أرجع resultp;  // Returning input so we can tail call this method
    }
};

#د_که_پای  // Guard
