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

#నిర్వచితం_కాకపోతే VERILATOR_V3GRAPHSTREAM_H_
#నిర్వచించు VERILATOR_V3GRAPHSTREAM_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Graph.h"

#అన్తర్భావ <functional>
#అన్తర్భావ <map>
#అన్తర్భావ <set>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <vector>

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

మూస <typename T_Compare>
class లేఖాచిత్రం_ప్రవాహం final {
    // TYPES
    class VxHolder final {
    బహిరంగ:
        // MEMBERS
        స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి m_vxp;  // [mtask] Vertex
        స్థిరరాశి uint32_t m_pos;  // Sort position
        uint32_t m_అంకె_Blocking_అంచులు;  // Number of blocking edges
        // CONSTRUCTORS
        VxHolder(స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* vxp, uint32_t pos, uint32_t అంకె_Blocking_అంచులు)
            : m_vxp{vxp}
            , m_pos{pos}
            , m_అంకె_Blocking_అంచులు{అంకె_Blocking_అంచులు} {}
        // METHODS
        స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* vertexp() స్థిరరాశి { తిరిగి m_vxp; }
        // Decrement blocking edges count, return true if the vertex is
        // newly unblocked
        bool unblock() {
            UASSERT_OBJ(m_అంకె_Blocking_అంచులు > 0, vertexp(), "Underflow of blocking edges");
            --m_అంకె_Blocking_అంచులు;
            తిరిగి (m_అంకె_Blocking_అంచులు == 0);
        }
    };

    class VxHolderCmp final {
    బహిరంగ:
        // MEMBERS
        స్థిరరాశి T_Compare m_lessThan;  // Sorting functor
        // CONSTRUCTORS
        explicit VxHolderCmp(స్థిరరాశి T_Compare& lessThan)
            : m_lessThan{lessThan} {}
        // METHODS
        bool క్రియాకారకం()(స్థిరరాశి VxHolder& a, స్థిరరాశి VxHolder& b) స్థిరరాశి {
            ఒకవేళ (m_lessThan.క్రియాకారకం()(a.vertexp(), b.vertexp())) తిరిగి true;
            ఒకవేళ (m_lessThan.క్రియాకారకం()(b.vertexp(), a.vertexp())) తిరిగి false;
            తిరిగి a.m_pos < b.m_pos;
        }

    అంతరంగ:
        VL_UNCOPYABLE(VxHolderCmp);
    };

    using ReadyVertices = std::సమితి<VxHolder, VxHolderCmp&>;

    // MEMBERS
    VxHolderCmp m_vxHolderCmp;  // Vertext comparison functor
    ReadyVertices m_readyVertices;  // List of ready vertices
    std::map<స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం*, VxHolder> m_నిరీక్షణ_Vertices;  // List of waiting vertices
    typename ReadyVertices::iterator m_చివరి;  // Previously returned element
    స్థిరరాశి లేఖాచిత్రం_Way m_way;  // FORWARD or REVERSE order of traversal

బహిరంగ:
    // CONSTRUCTORS
    explicit లేఖాచిత్రం_ప్రవాహం(స్థిరరాశి V3_లేఖాచిత్రం* graphp, లేఖాచిత్రం_Way way = లేఖాచిత్రం_Way::FORWARD,
                         స్థిరరాశి T_Compare& lessThan = T_Compare())
        // NOTE: Perhaps REVERSE way should also reverse the sense of the
        // lessThan function? For now the only usage of REVERSE is not
        // sensitive to its lessThan at all, so it doesn't matter.
        : m_vxHolderCmp{lessThan}
        , m_readyVertices{m_vxHolderCmp}
        , m_చివరి{m_readyVertices.చివర()}
        , m_way{way} {
        uint32_t pos = 0;
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : graphp->vertices()) {
            // Every vertex initially is waiting, or ready.
            ఒకవేళ (way == లేఖాచిత్రం_Way::FORWARD) {
                ఒకవేళ (vtx.లోపల_ఖాళీలు()) {
                    స్థిరరాశి VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.చొప్పించు(newVx);
                } లేకపోతే {
                    స్థిరరాశి uint32_t dep_లెక్క = vtx.లోపల_అంచులు().కొలత();
                    స్థిరరాశి VxHolder newVx{&vtx, pos++, dep_లెక్క};
                    m_నిరీక్షణ_Vertices.emplace(&vtx, newVx);
                }
            } లేకపోతే {  //తిరగేయి
                ఒకవేళ (vtx.బహిర్గతం_ఖాళీలు()) {
                    స్థిరరాశి VxHolder newVx{&vtx, pos++, 0};
                    m_readyVertices.చొప్పించు(newVx);
                } లేకపోతే {
                    స్థిరరాశి uint32_t dep_లెక్క = vtx.బహిర్గతం_అంచులు().కొలత();
                    స్థిరరాశి VxHolder newVx{&vtx, pos++, dep_లెక్క};
                    m_నిరీక్షణ_Vertices.emplace(&vtx, newVx);
                }
            }
        }
    }
    ~లేఖాచిత్రం_ప్రవాహం() = అప్రమేయ;

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
    స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* nextp() {
        స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* resultp = nullptr;

        typename ReadyVertices::iterator curIt;
        ఒకవేళ (m_చివరి == m_readyVertices.చివర()) {
            // First call to nextp()
            curIt = m_readyVertices.begin();
        } లేకపోతే {
            // Subsequent call to nextp()
            curIt = m_చివరి;
            ++curIt;
            // Remove previously-returned element
            m_readyVertices.erase(m_చివరి);
            // Wrap curIt. Expect to wrap, and make another pass, to find
            // newly-ready elements that could have appeared ahead of the
            // m_last iterator
            ఒకవేళ (curIt == m_readyVertices.చివర()) curIt = m_readyVertices.begin();
        }

        ఒకవేళ (curIt != m_readyVertices.చివర()) {
            resultp = curIt->vertexp();
            unblockDeps(resultp);
        } లేకపోతే {
            // No ready vertices; waiting should be empty too, otherwise we
            // were fed a graph with cycles (which is not supported.)
            UASSERT(m_నిరీక్షణ_Vertices.ఖాళీలు(), "DGS fed non-DAG");
        }

        m_చివరి = curIt;
        తిరిగి resultp;
    }

అంతరంగ:
    శూన్య unblockDeps(స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* vertexp) {
        ఒకవేళ (m_way == లేఖాచిత్రం_Way::FORWARD) {
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& edgep : vertexp->బహిర్గతం_అంచులు()) {
                స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి గమ్యం_Vertexp = edgep.శిఖరం();

                స్థిరరాశి స్వయంచాలక it = m_నిరీక్షణ_Vertices.కనుగొను(గమ్యం_Vertexp);
                UASSERT_OBJ(it != m_నిరీక్షణ_Vertices.చివర(), గమ్యం_Vertexp,
                            "Found edge into vertex not in waiting list.");
                ఒకవేళ (it->రెండవ.unblock()) {
                    m_readyVertices.చొప్పించు(it->రెండవ);
                    m_నిరీక్షణ_Vertices.erase(it);
                }
            }
        } లేకపోతే {
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : vertexp->లోపల_అంచులు()) {
                స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి fromVertexp = అంచు.fromp();

                స్థిరరాశి స్వయంచాలక it = m_నిరీక్షణ_Vertices.కనుగొను(fromVertexp);
                UASSERT_OBJ(it != m_నిరీక్షణ_Vertices.చివర(), fromVertexp,
                            "Found edge into vertex not in waiting list.");
                ఒకవేళ (it->రెండవ.unblock()) {
                    m_readyVertices.చొప్పించు(it->రెండవ);
                    m_నిరీక్షణ_Vertices.erase(it);
                }
            }
        }
    }

    VL_UNCOPYABLE(లేఖాచిత్రం_ప్రవాహం);
};

//=================================================================================================
// GraphStreamUnordered is similar to GraphStream, but iterates un-ordered vertices (those that are
// not ordered by dependencies) in an arbitrary order. Iteration order is still deterministic.

class లేఖాచిత్రం_ప్రవాహం_Unordered final {
    // MEMBERS
    స్థిరరాశి లేఖాచిత్రం_Way m_way;  // Direction of traversal
    కొలత_t m_తదుపరి_సూచీ = 0;  // Which index to return from m_nextVertices next
    std::సదిశ<స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం*> m_తదుపరి_Vertices;  // List of ready vertices returned next
    std::సదిశ<స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం*> m_readyVertices;  // List of other ready vertices

బహిరంగ:
    // CONSTRUCTORS
    VL_UNCOPYABLE(లేఖాచిత్రం_ప్రవాహం_Unordered);
    explicit లేఖాచిత్రం_ప్రవాహం_Unordered(V3_లేఖాచిత్రం* graphp, లేఖాచిత్రం_Way way = లేఖాచిత్రం_Way::FORWARD)
        : m_way{way} {
        ఒకవేళ (m_way == లేఖాచిత్రం_Way::FORWARD) {
            init<లేఖాచిత్రం_Way::FORWARD>(graphp);
        } లేకపోతే {
            init<లేఖాచిత్రం_Way::తిరగేయి_2>(graphp);
        }
    }
    ~లేఖాచిత్రం_ప్రవాహం_Unordered() = అప్రమేయ;

    // METHODS

    // Each call to nextp() returns a unique vertex in the graph, in dependency order. Dependencies
    // alone do not specify a total ordering. Un-ordered vertices are returned in an arbitrary but
    // deterministic order.
    స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* nextp() {
        ఒకవేళ (VL_UNLIKELY(m_తదుపరి_సూచీ == m_తదుపరి_Vertices.కొలత())) {
            ఒకవేళ (VL_UNLIKELY(m_readyVertices.ఖాళీలు())) తిరిగి nullptr;
            m_తదుపరి_సూచీ = 0;
            // Use swap to avoid reallocation
            m_తదుపరి_Vertices.మార్పిడి(m_readyVertices);
            m_readyVertices.clear();
        }
        స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి resultp = m_తదుపరి_Vertices[m_తదుపరి_సూచీ++];
        ఒకవేళ (m_way == లేఖాచిత్రం_Way::FORWARD) తిరిగి unblock<లేఖాచిత్రం_Way::FORWARD>(resultp);
        తిరిగి unblock<లేఖాచిత్రం_Way::తిరగేయి_2>(resultp);
    }

అంతరంగ:
    మూస <uint8_t N_Way>  //
    VL_ATTR_NOINLINE శూన్య init(V3_లేఖాచిత్రం* graphp) {
        constexpr లేఖాచిత్రం_Way way{N_Way};
        // Assign every vertex without an incoming edge to ready, others to waiting
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& శీర్షం : graphp->vertices()) {
            స్థిరరాశి uint32_t nDeps = శీర్షం.అంచులు<way.invert()>().కొలత();
            శీర్షం.color(nDeps);  // Using color instead of user, as user might be used by client
            ఒకవేళ (VL_UNLIKELY(nDeps == 0)) m_తదుపరి_Vertices.నెట్టు_వెనుక(&శీర్షం);
        }
    }

    మూస <uint8_t N_Way>  //
    VL_ATTR_NOINLINE స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* unblock(స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* resultp) {
        constexpr లేఖాచిత్రం_Way way{N_Way};
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : resultp->అంచులు<way>()) {
            V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vertexp = అంచు.furtherp<way>();
#ఒకవేళ VL_DEBUG
            UASSERT_OBJ(vertexp->color() != 0, vertexp, "Should not be on waiting list");
#అయితే_ముగింపు
            vertexp->color(vertexp->color() - 1);
            ఒకవేళ (!vertexp->color()) m_readyVertices.నెట్టు_వెనుక(vertexp);
        }
        తిరిగి resultp;  // Returning input so we can tail call this method
    }
};

#అయితే_ముగింపు  // Guard
