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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3GRAPHSTREAM_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3GRAPHSTREAM_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Graph.h"

#ಅನ್ತರ್ಭಾವ <functional>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

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

ಮಾದರಿ <typename T_Compare>
class ನಕ್ಷೆ_ಹರಿವು final {
    // TYPES
    class VxHolder final {
    ಸಾರ್ವಜನಿಕ:
        // MEMBERS
        ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ m_vxp;  // [mtask] Vertex
        ಸ್ಥಿರಾಂಕ uint32_t m_pos;  // Sort position
        uint32_t m_ಅಂಕೆ_Blocking_ಅಂಚುಗಳು;  // Number of blocking edges
        // CONSTRUCTORS
        VxHolder(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* vxp, uint32_t pos, uint32_t ಅಂಕೆ_Blocking_ಅಂಚುಗಳು)
            : m_vxp{vxp}
            , m_pos{pos}
            , m_ಅಂಕೆ_Blocking_ಅಂಚುಗಳು{ಅಂಕೆ_Blocking_ಅಂಚುಗಳು} {}
        // METHODS
        ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_vxp; }
        // Decrement blocking edges count, return true if the vertex is
        // newly unblocked
        bool unblock() {
            UASSERT_OBJ(m_ಅಂಕೆ_Blocking_ಅಂಚುಗಳು > 0, vertexp(), "Underflow of blocking edges");
            --m_ಅಂಕೆ_Blocking_ಅಂಚುಗಳು;
            ಹಿಂತಿರುಗಿ (m_ಅಂಕೆ_Blocking_ಅಂಚುಗಳು == 0);
        }
    };

    class VxHolderCmp final {
    ಸಾರ್ವಜನಿಕ:
        // MEMBERS
        ಸ್ಥಿರಾಂಕ T_Compare m_lessThan;  // Sorting functor
        // CONSTRUCTORS
        explicit VxHolderCmp(ಸ್ಥಿರಾಂಕ T_Compare& lessThan)
            : m_lessThan{lessThan} {}
        // METHODS
        bool ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ VxHolder& a, ಸ್ಥಿರಾಂಕ VxHolder& b) ಸ್ಥಿರಾಂಕ {
            ಆದರೆ (m_lessThan.ಕ್ರಿಯಾಕಾರಕ()(a.vertexp(), b.vertexp())) ಹಿಂತಿರುಗಿ true;
            ಆದರೆ (m_lessThan.ಕ್ರಿಯಾಕಾರಕ()(b.vertexp(), a.vertexp())) ಹಿಂತಿರುಗಿ false;
            ಹಿಂತಿರುಗಿ a.m_pos < b.m_pos;
        }

    ಖಾಸಗಿ:
        VL_UNCOPYABLE(VxHolderCmp);
    };

    using ReadyVertices = std::ಗಣ<VxHolder, VxHolderCmp&>;

    // MEMBERS
    VxHolderCmp m_vxHolderCmp;  // Vertext comparison functor
    ReadyVertices m_readyVertices;  // List of ready vertices
    std::map<ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ*, VxHolder> m_ಕಾಯುವಿಕೆ_Vertices;  // List of waiting vertices
    typename ReadyVertices::iterator m_ಕೊನೆಯ;  // Previously returned element
    ಸ್ಥಿರಾಂಕ ನಕ್ಷೆ_Way m_way;  // FORWARD or REVERSE order of traversal

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit ನಕ್ಷೆ_ಹರಿವು(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ* graphp, ನಕ್ಷೆ_Way way = ನಕ್ಷೆ_Way::FORWARD,
                         ಸ್ಥಿರಾಂಕ T_Compare& lessThan = T_Compare())
        // NOTE: Perhaps REVERSE way should also reverse the sense of the
        // lessThan function? For now the only usage of REVERSE is not
        // sensitive to its lessThan at all, so it doesn't matter.
        : m_vxHolderCmp{lessThan}
        , m_readyVertices{m_vxHolderCmp}
        , m_ಕೊನೆಯ{m_readyVertices.ಕೊನೆ()}
        , m_way{way} {
        uint32_t pos = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : graphp->vertices()) {
            // Every vertex initially is waiting, or ready.
            ಆದರೆ (way == ನಕ್ಷೆ_Way::FORWARD) {
                ಆದರೆ (vtx.ಒಳ_ಖಾಲಿಗಳು()) {
                    ಸ್ಥಿರಾಂಕ VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.ಸೇರಿಸು(newVx);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಸ್ಥಿರಾಂಕ uint32_t dep_ಎಣಿಕೆ = vtx.ಒಳ_ಅಂಚುಗಳು().ಗಾತ್ರ_2();
                    ಸ್ಥಿರಾಂಕ VxHolder newVx{&vtx, pos++, dep_ಎಣಿಕೆ};
                    m_ಕಾಯುವಿಕೆ_Vertices.emplace(&vtx, newVx);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {  //ಹಿಮ್ಮುಖ
                ಆದರೆ (vtx.ಹೊರಹೋಗು_ಖಾಲಿಗಳು()) {
                    ಸ್ಥಿರಾಂಕ VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.ಸೇರಿಸು(newVx);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಸ್ಥಿರಾಂಕ uint32_t dep_ಎಣಿಕೆ = vtx.ಹೊರಹೋಗು_ಅಂಚುಗಳು().ಗಾತ್ರ_2();
                    ಸ್ಥಿರಾಂಕ VxHolder newVx{&vtx, pos++, dep_ಎಣಿಕೆ};
                    m_ಕಾಯುವಿಕೆ_Vertices.emplace(&vtx, newVx);
                }
            }
        }
    }
    ~ನಕ್ಷೆ_ಹರಿವು() = ಪೂರ್ವನಿಯೋಜಿತ;

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
    ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* nextp() {
        ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* resultp = nullptr;

        typename ReadyVertices::iterator curIt;
        ಆದರೆ (m_ಕೊನೆಯ == m_readyVertices.ಕೊನೆ()) {
            // First call to nextp()
            curIt = m_readyVertices.begin();
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Subsequent call to nextp()
            curIt = m_ಕೊನೆಯ;
            ++curIt;
            // Remove previously-returned element
            m_readyVertices.erase(m_ಕೊನೆಯ);
            // Wrap curIt. Expect to wrap, and make another pass, to find
            // newly-ready elements that could have appeared ahead of the
            // m_last iterator
            ಆದರೆ (curIt == m_readyVertices.ಕೊನೆ()) curIt = m_readyVertices.begin();
        }

        ಆದರೆ (curIt != m_readyVertices.ಕೊನೆ()) {
            resultp = curIt->vertexp();
            unblockDeps(resultp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // No ready vertices; waiting should be empty too, otherwise we
            // were fed a graph with cycles (which is not supported.)
            UASSERT(m_ಕಾಯುವಿಕೆ_Vertices.ಖಾಲಿಗಳು(), "DGS fed non-DAG");
        }

        m_ಕೊನೆಯ = curIt;
        ಹಿಂತಿರುಗಿ resultp;
    }

ಖಾಸಗಿ:
    ಶೂನ್ಯ unblockDeps(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp) {
        ಆದರೆ (m_way == ನಕ್ಷೆ_Way::FORWARD) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& edgep : vertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ಗಮ್ಯ_Vertexp = edgep.ತುದಿ();

                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_ಕಾಯುವಿಕೆ_Vertices.ಕಂಡುಹಿಡಿ(ಗಮ್ಯ_Vertexp);
                UASSERT_OBJ(it != m_ಕಾಯುವಿಕೆ_Vertices.ಕೊನೆ(), ಗಮ್ಯ_Vertexp,
                            "Found edge into vertex not in waiting list.");
                ಆದರೆ (it->ಎರಡನೇ.unblock()) {
                    m_readyVertices.ಸೇರಿಸು(it->ಎರಡನೇ);
                    m_ಕಾಯುವಿಕೆ_Vertices.erase(it);
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vertexp->ಒಳ_ಅಂಚುಗಳು()) {
                ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fromVertexp = ಅಂಚು.fromp();

                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_ಕಾಯುವಿಕೆ_Vertices.ಕಂಡುಹಿಡಿ(fromVertexp);
                UASSERT_OBJ(it != m_ಕಾಯುವಿಕೆ_Vertices.ಕೊನೆ(), fromVertexp,
                            "Found edge into vertex not in waiting list.");
                ಆದರೆ (it->ಎರಡನೇ.unblock()) {
                    m_readyVertices.ಸೇರಿಸು(it->ಎರಡನೇ);
                    m_ಕಾಯುವಿಕೆ_Vertices.erase(it);
                }
            }
        }
    }

    VL_UNCOPYABLE(ನಕ್ಷೆ_ಹರಿವು);
};

//=================================================================================================
// GraphStreamUnordered is similar to GraphStream, but iterates un-ordered vertices (those that are
// not ordered by dependencies) in an arbitrary order. Iteration order is still deterministic.

class ನಕ್ಷೆ_ಹರಿವು_Unordered final {
    // MEMBERS
    ಸ್ಥಿರಾಂಕ ನಕ್ಷೆ_Way m_way;  // Direction of traversal
    ಗಾತ್ರ_t m_ಮುಂದಿನ_ಸೂಚ್ಯಂಕ = 0;  // Which index to return from m_nextVertices next
    std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ*> m_ಮುಂದಿನ_Vertices;  // List of ready vertices returned next
    std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ*> m_readyVertices;  // List of other ready vertices

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    VL_UNCOPYABLE(ನಕ್ಷೆ_ಹರಿವು_Unordered);
    explicit ನಕ್ಷೆ_ಹರಿವು_Unordered(V3_ನಕ್ಷೆ* graphp, ನಕ್ಷೆ_Way way = ನಕ್ಷೆ_Way::FORWARD)
        : m_way{way} {
        ಆದರೆ (m_way == ನಕ್ಷೆ_Way::FORWARD) {
            init<ನಕ್ಷೆ_Way::FORWARD>(graphp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            init<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>(graphp);
        }
    }
    ~ನಕ್ಷೆ_ಹರಿವು_Unordered() = ಪೂರ್ವನಿಯೋಜಿತ;

    // METHODS

    // Each call to nextp() returns a unique vertex in the graph, in dependency order. Dependencies
    // alone do not specify a total ordering. Un-ordered vertices are returned in an arbitrary but
    // deterministic order.
    ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* nextp() {
        ಆದರೆ (VL_UNLIKELY(m_ಮುಂದಿನ_ಸೂಚ್ಯಂಕ == m_ಮುಂದಿನ_Vertices.ಗಾತ್ರ_2())) {
            ಆದರೆ (VL_UNLIKELY(m_readyVertices.ಖಾಲಿಗಳು())) ಹಿಂತಿರುಗಿ nullptr;
            m_ಮುಂದಿನ_ಸೂಚ್ಯಂಕ = 0;
            // Use swap to avoid reallocation
            m_ಮುಂದಿನ_Vertices.ಅದಲುಬದಲು(m_readyVertices);
            m_readyVertices.clear();
        }
        ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ resultp = m_ಮುಂದಿನ_Vertices[m_ಮುಂದಿನ_ಸೂಚ್ಯಂಕ++];
        ಆದರೆ (m_way == ನಕ್ಷೆ_Way::FORWARD) ಹಿಂತಿರುಗಿ unblock<ನಕ್ಷೆ_Way::FORWARD>(resultp);
        ಹಿಂತಿರುಗಿ unblock<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>(resultp);
    }

ಖಾಸಗಿ:
    ಮಾದರಿ <uint8_t N_Way>  //
    VL_ATTR_NOINLINE ಶೂನ್ಯ init(V3_ನಕ್ಷೆ* graphp) {
        constexpr ನಕ್ಷೆ_Way way{N_Way};
        // Assign every vertex without an incoming edge to ready, others to waiting
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : graphp->vertices()) {
            ಸ್ಥಿರಾಂಕ uint32_t nDeps = ಶೃಂಗ.ಅಂಚುಗಳು<way.invert()>().ಗಾತ್ರ_2();
            ಶೃಂಗ.color(nDeps);  // Using color instead of user, as user might be used by client
            ಆದರೆ (VL_UNLIKELY(nDeps == 0)) m_ಮುಂದಿನ_Vertices.ತಳ್ಳು_ಹಿಂಭಾಗ(&ಶೃಂಗ);
        }
    }

    ಮಾದರಿ <uint8_t N_Way>  //
    VL_ATTR_NOINLINE ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* unblock(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* resultp) {
        constexpr ನಕ್ಷೆ_Way way{N_Way};
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : resultp->ಅಂಚುಗಳು<way>()) {
            V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vertexp = ಅಂಚು.furtherp<way>();
#ಆದರೆ VL_DEBUG
            UASSERT_OBJ(vertexp->color() != 0, vertexp, "Should not be on waiting list");
#ಆದರೆ_ಅಂತ್ಯ
            vertexp->color(vertexp->color() - 1);
            ಆದರೆ (!vertexp->color()) m_readyVertices.ತಳ್ಳು_ಹಿಂಭಾಗ(vertexp);
        }
        ಹಿಂತಿರುಗಿ resultp;  // Returning input so we can tail call this method
    }
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
