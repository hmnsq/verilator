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

#यदि_अपरिभाषितम् VERILATOR_V3GRAPHSTREAM_H_
#परिभाषय VERILATOR_V3GRAPHSTREAM_H_

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

प्रतिरूपम् <typename T_Compare>
class Graph_प्रवाहः final {
    // TYPES
    class VxHolder final {
    सार्वजनिक:
        // MEMBERS
        ध्रुव V3GraphVertex* ध्रुव m_vxp;  // [mtask] Vertex
        ध्रुव uint32_t m_pos;  // Sort position
        uint32_t m_अङ्कः_Blocking_धाराः;  // Number of blocking edges
        // CONSTRUCTORS
        VxHolder(ध्रुव V3GraphVertex* vxp, uint32_t pos, uint32_t अङ्कः_Blocking_धाराः)
            : m_vxp{vxp}
            , m_pos{pos}
            , m_अङ्कः_Blocking_धाराः{अङ्कः_Blocking_धाराः} {}
        // METHODS
        ध्रुव V3GraphVertex* vertexp() ध्रुव { प्रत्यागम m_vxp; }
        // Decrement blocking edges count, return true if the vertex is
        // newly unblocked
        bool unblock() {
            UASSERT_OBJ(m_अङ्कः_Blocking_धाराः > 0, vertexp(), "Underflow of blocking edges");
            --m_अङ्कः_Blocking_धाराः;
            प्रत्यागम (m_अङ्कः_Blocking_धाराः == 0);
        }
    };

    class VxHolderCmp final {
    सार्वजनिक:
        // MEMBERS
        ध्रुव T_Compare m_lessThan;  // Sorting functor
        // CONSTRUCTORS
        explicit VxHolderCmp(ध्रुव T_Compare& lessThan)
            : m_lessThan{lessThan} {}
        // METHODS
        bool संक्रियकः()(ध्रुव VxHolder& a, ध्रुव VxHolder& b) ध्रुव {
            यदि (m_lessThan.संक्रियकः()(a.vertexp(), b.vertexp())) प्रत्यागम true;
            यदि (m_lessThan.संक्रियकः()(b.vertexp(), a.vertexp())) प्रत्यागम false;
            प्रत्यागम a.m_pos < b.m_pos;
        }

    गुप्त:
        VL_UNCOPYABLE(VxHolderCmp);
    };

    using ReadyVertices = std::समुच्चयः<VxHolder, VxHolderCmp&>;

    // MEMBERS
    VxHolderCmp m_vxHolderCmp;  // Vertext comparison functor
    ReadyVertices m_readyVertices;  // List of ready vertices
    std::map<ध्रुव V3GraphVertex*, VxHolder> m_प्रतीक्षा_Vertices;  // List of waiting vertices
    typename ReadyVertices::iterator m_चरमः;  // Previously returned element
    ध्रुव GraphWay m_way;  // FORWARD or REVERSE order of traversal

सार्वजनिक:
    // CONSTRUCTORS
    explicit Graph_प्रवाहः(ध्रुव V3Graph* graphp, GraphWay way = GraphWay::FORWARD,
                         ध्रुव T_Compare& lessThan = T_Compare())
        // NOTE: Perhaps REVERSE way should also reverse the sense of the
        // lessThan function? For now the only usage of REVERSE is not
        // sensitive to its lessThan at all, so it doesn't matter.
        : m_vxHolderCmp{lessThan}
        , m_readyVertices{m_vxHolderCmp}
        , m_चरमः{m_readyVertices.अन्तः_2()}
        , m_way{way} {
        uint32_t pos = 0;
        प्रति (ध्रुव V3GraphVertex& vtx : graphp->vertices()) {
            // Every vertex initially is waiting, or ready.
            यदि (way == GraphWay::FORWARD) {
                यदि (vtx.अन्तर्_रिक्तानि()) {
                    ध्रुव VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.निवेशय(newVx);
                } अन्यथा {
                    ध्रुव uint32_t dep_गणनम् = vtx.अन्तर्_धाराः().आकार();
                    ध्रुव VxHolder newVx{&vtx, pos++, dep_गणनम्};
                    m_प्रतीक्षा_Vertices.emplace(&vtx, newVx);
                }
            } अन्यथा {  // REVERSE
                यदि (vtx.बहिर्गमनम्_रिक्तानि()) {
                    ध्रुव VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.निवेशय(newVx);
                } अन्यथा {
                    ध्रुव uint32_t dep_गणनम् = vtx.बहिर्गमनम्_धाराः().आकार();
                    ध्रुव VxHolder newVx{&vtx, pos++, dep_गणनम्};
                    m_प्रतीक्षा_Vertices.emplace(&vtx, newVx);
                }
            }
        }
    }
    ~Graph_प्रवाहः() = स्वभाव;

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
    ध्रुव V3GraphVertex* nextp() {
        ध्रुव V3GraphVertex* resultp = nullptr;

        typename ReadyVertices::iterator curIt;
        यदि (m_चरमः == m_readyVertices.अन्तः_2()) {
            // First call to nextp()
            curIt = m_readyVertices.begin();
        } अन्यथा {
            // Subsequent call to nextp()
            curIt = m_चरमः;
            ++curIt;
            // Remove previously-returned element
            m_readyVertices.erase(m_चरमः);
            // Wrap curIt. Expect to wrap, and make another pass, to find
            // newly-ready elements that could have appeared ahead of the
            // m_last iterator
            यदि (curIt == m_readyVertices.अन्तः_2()) curIt = m_readyVertices.begin();
        }

        यदि (curIt != m_readyVertices.अन्तः_2()) {
            resultp = curIt->vertexp();
            unblockDeps(resultp);
        } अन्यथा {
            // No ready vertices; waiting should be empty too, otherwise we
            // were fed a graph with cycles (which is not supported.)
            UASSERT(m_प्रतीक्षा_Vertices.रिक्तानि(), "DGS fed non-DAG");
        }

        m_चरमः = curIt;
        प्रत्यागम resultp;
    }

गुप्त:
    शून्य unblockDeps(ध्रुव V3GraphVertex* vertexp) {
        यदि (m_way == GraphWay::FORWARD) {
            प्रति (ध्रुव V3GraphEdge& edgep : vertexp->बहिर्गमनम्_धाराः()) {
                ध्रुव V3GraphVertex* ध्रुव गन्तव्यम्_Vertexp = edgep.शिखरम्();

                ध्रुव स्वचालित it = m_प्रतीक्षा_Vertices.find(गन्तव्यम्_Vertexp);
                UASSERT_OBJ(it != m_प्रतीक्षा_Vertices.अन्तः_2(), गन्तव्यम्_Vertexp,
                            "Found edge into vertex not in waiting list.");
                यदि (it->second.unblock()) {
                    m_readyVertices.निवेशय(it->second);
                    m_प्रतीक्षा_Vertices.erase(it);
                }
            }
        } अन्यथा {
            प्रति (ध्रुव V3GraphEdge& edge : vertexp->अन्तर्_धाराः()) {
                ध्रुव V3GraphVertex* ध्रुव fromVertexp = edge.fromp();

                ध्रुव स्वचालित it = m_प्रतीक्षा_Vertices.find(fromVertexp);
                UASSERT_OBJ(it != m_प्रतीक्षा_Vertices.अन्तः_2(), fromVertexp,
                            "Found edge into vertex not in waiting list.");
                यदि (it->second.unblock()) {
                    m_readyVertices.निवेशय(it->second);
                    m_प्रतीक्षा_Vertices.erase(it);
                }
            }
        }
    }

    VL_UNCOPYABLE(Graph_प्रवाहः);
};

//=================================================================================================
// GraphStreamUnordered is similar to GraphStream, but iterates un-ordered vertices (those that are
// not ordered by dependencies) in an arbitrary order. Iteration order is still deterministic.

class Graph_प्रवाहः_Unordered final {
    // MEMBERS
    ध्रुव GraphWay m_way;  // Direction of traversal
    आकार_t m_अग्र_सूचकांक = 0;  // Which index to return from m_nextVertices next
    std::सदिशम्<ध्रुव V3GraphVertex*> m_अग्र_Vertices;  // List of ready vertices returned next
    std::सदिशम्<ध्रुव V3GraphVertex*> m_readyVertices;  // List of other ready vertices

सार्वजनिक:
    // CONSTRUCTORS
    VL_UNCOPYABLE(Graph_प्रवाहः_Unordered);
    explicit Graph_प्रवाहः_Unordered(V3Graph* graphp, GraphWay way = GraphWay::FORWARD)
        : m_way{way} {
        यदि (m_way == GraphWay::FORWARD) {
            init<GraphWay::FORWARD>(graphp);
        } अन्यथा {
            init<GraphWay::REVERSE>(graphp);
        }
    }
    ~Graph_प्रवाहः_Unordered() = स्वभाव;

    // METHODS

    // Each call to nextp() returns a unique vertex in the graph, in dependency order. Dependencies
    // alone do not specify a total ordering. Un-ordered vertices are returned in an arbitrary but
    // deterministic order.
    ध्रुव V3GraphVertex* nextp() {
        यदि (VL_UNLIKELY(m_अग्र_सूचकांक == m_अग्र_Vertices.आकार())) {
            यदि (VL_UNLIKELY(m_readyVertices.रिक्तानि())) प्रत्यागम nullptr;
            m_अग्र_सूचकांक = 0;
            // Use swap to avoid reallocation
            m_अग्र_Vertices.swap(m_readyVertices);
            m_readyVertices.clear();
        }
        ध्रुव V3GraphVertex* ध्रुव resultp = m_अग्र_Vertices[m_अग्र_सूचकांक++];
        यदि (m_way == GraphWay::FORWARD) प्रत्यागम unblock<GraphWay::FORWARD>(resultp);
        प्रत्यागम unblock<GraphWay::REVERSE>(resultp);
    }

गुप्त:
    प्रतिरूपम् <uint8_t N_Way>  //
    VL_ATTR_NOINLINE शून्य init(V3Graph* graphp) {
        constexpr GraphWay way{N_Way};
        // Assign every vertex without an incoming edge to ready, others to waiting
        प्रति (V3GraphVertex& vertex : graphp->vertices()) {
            ध्रुव uint32_t nDeps = vertex.धाराः<way.invert()>().आकार();
            vertex.color(nDeps);  // Using color instead of user, as user might be used by client
            यदि (VL_UNLIKELY(nDeps == 0)) m_अग्र_Vertices.प्रक्षिप_पृष्ठभागः(&vertex);
        }
    }

    प्रतिरूपम् <uint8_t N_Way>  //
    VL_ATTR_NOINLINE ध्रुव V3GraphVertex* unblock(ध्रुव V3GraphVertex* resultp) {
        constexpr GraphWay way{N_Way};
        प्रति (ध्रुव V3GraphEdge& edge : resultp->धाराः<way>()) {
            V3GraphVertex* ध्रुव vertexp = edge.furtherp<way>();
#यदि VL_DEBUG
            UASSERT_OBJ(vertexp->color() != 0, vertexp, "Should not be on waiting list");
#यदि_अन्तः
            vertexp->color(vertexp->color() - 1);
            यदि (!vertexp->color()) m_readyVertices.प्रक्षिप_पृष्ठभागः(vertexp);
        }
        प्रत्यागम resultp;  // Returning input so we can tail call this method
    }
};

#यदि_अन्तः  // Guard
