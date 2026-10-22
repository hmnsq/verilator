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

#جے_بنا_تعریف VERILATOR_V3GRAPHSTREAM_H_
#تعریف_کرو VERILATOR_V3GRAPHSTREAM_H_

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

سانچا <typename T_Compare>
class گراف_دھارا final {
    // TYPES
    class VxHolder final {
    عام:
        // MEMBERS
        ثابت V3_گراف_راس* ثابت m_vxp;  // [mtask] Vertex
        ثابت uint32_t m_pos;  // Sort position
        uint32_t m_عدد_Blocking_کنارے;  // Number of blocking edges
        // CONSTRUCTORS
        VxHolder(ثابت V3_گراف_راس* vxp, uint32_t pos, uint32_t عدد_Blocking_کنارے)
            : m_vxp{vxp}
            , m_pos{pos}
            , m_عدد_Blocking_کنارے{عدد_Blocking_کنارے} {}
        // METHODS
        ثابت V3_گراف_راس* vertexp() ثابت { أرجع m_vxp; }
        // Decrement blocking edges count, return true if the vertex is
        // newly unblocked
        bool unblock() {
            UASSERT_OBJ(m_عدد_Blocking_کنارے > 0, vertexp(), "Underflow of blocking edges");
            --m_عدد_Blocking_کنارے;
            أرجع (m_عدد_Blocking_کنارے == 0);
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

    using ReadyVertices = std::سیٹ<VxHolder, VxHolderCmp&>;

    // MEMBERS
    VxHolderCmp m_vxHolderCmp;  // Vertext comparison functor
    ReadyVertices m_readyVertices;  // List of ready vertices
    std::map<ثابت V3_گراف_راس*, VxHolder> m_اڈیک_Vertices;  // List of waiting vertices
    typename ReadyVertices::iterator m_آخری;  // Previously returned element
    ثابت گراف_Way m_way;  // FORWARD or REVERSE order of traversal

عام:
    // CONSTRUCTORS
    explicit گراف_دھارا(ثابت V3_گراف* graphp, گراف_Way way = گراف_Way::FORWARD,
                         ثابت T_Compare& lessThan = T_Compare())
        // NOTE: Perhaps REVERSE way should also reverse the sense of the
        // lessThan function? For now the only usage of REVERSE is not
        // sensitive to its lessThan at all, so it doesn't matter.
        : m_vxHolderCmp{lessThan}
        , m_readyVertices{m_vxHolderCmp}
        , m_آخری{m_readyVertices.سرا()}
        , m_way{way} {
        uint32_t pos = 0;
        لكل (ثابت V3_گراف_راس& vtx : graphp->vertices()) {
            // Every vertex initially is waiting, or ready.
            إذا (way == گراف_Way::FORWARD) {
                إذا (vtx.وچ_خالی_تھاواں()) {
                    ثابت VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.داخل_کرو(newVx);
                } وإلا {
                    ثابت uint32_t dep_گنتی = vtx.وچ_کنارے().ناپ();
                    ثابت VxHolder newVx{&vtx, pos++, dep_گنتی};
                    m_اڈیک_Vertices.emplace(&vtx, newVx);
                }
            } وإلا {  //الٹ
                إذا (vtx.باہرلا_خالی_تھاواں()) {
                    ثابت VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.داخل_کرو(newVx);
                } وإلا {
                    ثابت uint32_t dep_گنتی = vtx.باہرلا_کنارے().ناپ();
                    ثابت VxHolder newVx{&vtx, pos++, dep_گنتی};
                    m_اڈیک_Vertices.emplace(&vtx, newVx);
                }
            }
        }
    }
    ~گراف_دھارا() = افتراضي;

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
        إذا (m_آخری == m_readyVertices.سرا()) {
            // First call to nextp()
            curIt = m_readyVertices.begin();
        } وإلا {
            // Subsequent call to nextp()
            curIt = m_آخری;
            ++curIt;
            // Remove previously-returned element
            m_readyVertices.erase(m_آخری);
            // Wrap curIt. Expect to wrap, and make another pass, to find
            // newly-ready elements that could have appeared ahead of the
            // m_last iterator
            إذا (curIt == m_readyVertices.سرا()) curIt = m_readyVertices.begin();
        }

        إذا (curIt != m_readyVertices.سرا()) {
            resultp = curIt->vertexp();
            unblockDeps(resultp);
        } وإلا {
            // No ready vertices; waiting should be empty too, otherwise we
            // were fed a graph with cycles (which is not supported.)
            UASSERT(m_اڈیک_Vertices.خالی_تھاواں(), "DGS fed non-DAG");
        }

        m_آخری = curIt;
        أرجع resultp;
    }

خاص:
    فراغ unblockDeps(ثابت V3_گراف_راس* vertexp) {
        إذا (m_way == گراف_Way::FORWARD) {
            لكل (ثابت V3_گراف_کنارا& edgep : vertexp->باہرلا_کنارے()) {
                ثابت V3_گراف_راس* ثابت منزل_Vertexp = edgep.چوٹی();

                ثابت خودکار it = m_اڈیک_Vertices.لبھو(منزل_Vertexp);
                UASSERT_OBJ(it != m_اڈیک_Vertices.سرا(), منزل_Vertexp,
                            "Found edge into vertex not in waiting list.");
                إذا (it->دوجا.unblock()) {
                    m_readyVertices.داخل_کرو(it->دوجا);
                    m_اڈیک_Vertices.erase(it);
                }
            }
        } وإلا {
            لكل (ثابت V3_گراف_کنارا& کنارا : vertexp->وچ_کنارے()) {
                ثابت V3_گراف_راس* ثابت fromVertexp = کنارا.fromp();

                ثابت خودکار it = m_اڈیک_Vertices.لبھو(fromVertexp);
                UASSERT_OBJ(it != m_اڈیک_Vertices.سرا(), fromVertexp,
                            "Found edge into vertex not in waiting list.");
                إذا (it->دوجا.unblock()) {
                    m_readyVertices.داخل_کرو(it->دوجا);
                    m_اڈیک_Vertices.erase(it);
                }
            }
        }
    }

    VL_UNCOPYABLE(گراف_دھارا);
};

//=================================================================================================
// GraphStreamUnordered is similar to GraphStream, but iterates un-ordered vertices (those that are
// not ordered by dependencies) in an arbitrary order. Iteration order is still deterministic.

class گراف_دھارا_Unordered final {
    // MEMBERS
    ثابت گراف_Way m_way;  // Direction of traversal
    ناپ_t m_اگلا_اشاریہ = 0;  // Which index to return from m_nextVertices next
    std::سمتیہ<ثابت V3_گراف_راس*> m_اگلا_Vertices;  // List of ready vertices returned next
    std::سمتیہ<ثابت V3_گراف_راس*> m_readyVertices;  // List of other ready vertices

عام:
    // CONSTRUCTORS
    VL_UNCOPYABLE(گراف_دھارا_Unordered);
    explicit گراف_دھارا_Unordered(V3_گراف* graphp, گراف_Way way = گراف_Way::FORWARD)
        : m_way{way} {
        إذا (m_way == گراف_Way::FORWARD) {
            init<گراف_Way::FORWARD>(graphp);
        } وإلا {
            init<گراف_Way::الٹ_2>(graphp);
        }
    }
    ~گراف_دھارا_Unordered() = افتراضي;

    // METHODS

    // Each call to nextp() returns a unique vertex in the graph, in dependency order. Dependencies
    // alone do not specify a total ordering. Un-ordered vertices are returned in an arbitrary but
    // deterministic order.
    ثابت V3_گراف_راس* nextp() {
        إذا (VL_UNLIKELY(m_اگلا_اشاریہ == m_اگلا_Vertices.ناپ())) {
            إذا (VL_UNLIKELY(m_readyVertices.خالی_تھاواں())) أرجع nullptr;
            m_اگلا_اشاریہ = 0;
            // Use swap to avoid reallocation
            m_اگلا_Vertices.وٹاندرا(m_readyVertices);
            m_readyVertices.clear();
        }
        ثابت V3_گراف_راس* ثابت resultp = m_اگلا_Vertices[m_اگلا_اشاریہ++];
        إذا (m_way == گراف_Way::FORWARD) أرجع unblock<گراف_Way::FORWARD>(resultp);
        أرجع unblock<گراف_Way::الٹ_2>(resultp);
    }

خاص:
    سانچا <uint8_t N_Way>  //
    VL_ATTR_NOINLINE فراغ init(V3_گراف* graphp) {
        constexpr گراف_Way way{N_Way};
        // Assign every vertex without an incoming edge to ready, others to waiting
        لكل (V3_گراف_راس& راس : graphp->vertices()) {
            ثابت uint32_t nDeps = راس.کنارے<way.invert()>().ناپ();
            راس.color(nDeps);  // Using color instead of user, as user might be used by client
            إذا (VL_UNLIKELY(nDeps == 0)) m_اگلا_Vertices.ڈالو_پٹھ(&راس);
        }
    }

    سانچا <uint8_t N_Way>  //
    VL_ATTR_NOINLINE ثابت V3_گراف_راس* unblock(ثابت V3_گراف_راس* resultp) {
        constexpr گراف_Way way{N_Way};
        لكل (ثابت V3_گراف_کنارا& کنارا : resultp->کنارے<way>()) {
            V3_گراف_راس* ثابت vertexp = کنارا.furtherp<way>();
#إذا VL_DEBUG
            UASSERT_OBJ(vertexp->color() != 0, vertexp, "Should not be on waiting list");
#جے_انت
            vertexp->color(vertexp->color() - 1);
            إذا (!vertexp->color()) m_readyVertices.ڈالو_پٹھ(vertexp);
        }
        أرجع resultp;  // Returning input so we can tail call this method
    }
};

#جے_انت  // Guard
