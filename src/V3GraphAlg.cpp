// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Graph optimizations
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

#నిర్వచించు VL_MT_DISABLED_సంకేతం_UNIT 1

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3GraphAlg.h"

#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3GraphPathChecker.h"
#అన్తర్భావ "V3GraphStream.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <limits>
#అన్తర్భావ <list>
#అన్తర్భావ <map>
#అన్తర్భావ <numeric>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - Remove redundancies
// Changes user() and weight()

తరగతి లేఖాచిత్రం_Remove_Redundant final : లేఖాచిత్రం_Alg<> {
    స్థిరరాశి తార్కిక m_కూడిక_Weights;  // Sum, rather then maximize weights
అంతరంగ:
    శూన్య ప్రధాన() {
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& శీర్షం : m_graphp->vertices()) శీర్షం_Iterate(&శీర్షం);
    }
    శూన్య శీర్షం_Iterate(V3_లేఖాచిత్రం_శీర్షం* vertexp) {
        // Clear marks
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vertexp->బహిర్గతం_అంచులు()) అంచు.శిఖరం()->userp(శూన్యసూచిక);

        // Mark edges and detect duplications

        కొరకు (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep : vertexp->బహిర్గతం_అంచులు().unlinkable()) {
            ఒకవేళ (follow_అంచు(edgep)) {
                V3_లేఖాచిత్రం_శీర్షం* బహిర్గతం_Vertexp = edgep->శిఖరం();
                V3_లేఖాచిత్రం_అంచు* prevEdgep = static_cast<V3_లేఖాచిత్రం_అంచు*>(బహిర్గతం_Vertexp->userp());
                ఒకవేళ (!prevEdgep) {  // No previous assignment
                    బహిర్గతం_Vertexp->userp(edgep);
                } లేకపోతే {  // Duplicate
                    తార్కిక saveOld = నిజం;
                    ఒకవేళ (prevEdgep->cutable() && !edgep->cutable()) {
                        saveOld = అబద్ధం;  // new !cutable more important than old
                    } లేకపోతే ఒకవేళ (!prevEdgep->cutable() && edgep->cutable()) {
                        saveOld = నిజం;  // old !cutable more important than new
                    } లేకపోతే {
                        saveOld = నిజం;
                        ఒకవేళ (!m_కూడిక_Weights
                            && (prevEdgep->బరువు_2() < edgep->బరువు_2())) {  // Keep max weight
                            prevEdgep->బరువు_2(edgep->బరువు_2());
                        }
                    }
                    ఒకవేళ (saveOld) {
                        ఒకవేళ (m_కూడిక_Weights) prevEdgep->బరువు_2(prevEdgep->బరువు_2() + edgep->బరువు_2());
                        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    } లేకపోతే {
                        ఒకవేళ (m_కూడిక_Weights) edgep->బరువు_2(prevEdgep->బరువు_2() + edgep->బరువు_2());
                        VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                        బహిర్గతం_Vertexp->userp(edgep);
                    }
                }
            }
        }
    }

బహిరంగ:
    లేఖాచిత్రం_Remove_Redundant(V3_లేఖాచిత్రం* graphp, V3_అంచు_Func_P అంచు_Funcp, తార్కిక కూడిక_Weights)
        : లేఖాచిత్రం_Alg<>{graphp, అంచు_Funcp}
        , m_కూడిక_Weights{కూడిక_Weights} {
        ప్రధాన();
    }
    ~లేఖాచిత్రం_Remove_Redundant() = అప్రమేయ;
};

శూన్య V3_లేఖాచిత్రం::remove_Redundant_అంచులు_Max(V3_అంచు_Func_P అంచు_Funcp) {
    లేఖాచిత్రం_Remove_Redundant{ఇది, అంచు_Funcp, అబద్ధం};
}
శూన్య V3_లేఖాచిత్రం::remove_Redundant_అంచులు_కూడిక(V3_అంచు_Func_P అంచు_Funcp) {
    లేఖాచిత్రం_Remove_Redundant{ఇది, అంచు_Funcp, నిజం};
}

//######################################################################
//######################################################################
// Algorithms - remove transitive

తరగతి లేఖాచిత్రం_Alg_Remove_సంక్రమణ_అంచులు final : లేఖాచిత్రం_Alg<> {
బహిరంగ:
    స్పష్టమైన లేఖాచిత్రం_Alg_Remove_సంక్రమణ_అంచులు(V3_లేఖాచిత్రం* graphp)
        : లేఖాచిత్రం_Alg<>(graphp, శూన్యసూచిక) {}
    శూన్య go() {
        లేఖాచిత్రం_మార్గం_Checker checker{m_graphp};
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_graphp->vertices()) {
            V3_లేఖాచిత్రం_అంచు* deletep = శూన్యసూచిక;
            కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vtx.బహిర్గతం_అంచులు()) {
                ఒకవేళ (deletep) VL_DO_CLEAR(deletep->unlinkDelete(), deletep = శూన్యసూచిక);
                // It should be safe to modify the graph, despite using
                // the GraphPathChecker, as none of the modifications will
                // change what can be reached from what, nor should they
                // change the rank or CP of any node.
                ఒకవేళ (checker.is_సంక్రమణ_అంచు(&అంచు)) deletep = &అంచు;
            }
            ఒకవేళ (deletep) VL_DO_DANGLING(deletep->unlinkDelete(), deletep);
        }
    }

అంతరంగ:
    VL_UNCOPYABLE(లేఖాచిత్రం_Alg_Remove_సంక్రమణ_అంచులు);
};

శూన్య V3_లేఖాచిత్రం::remove_సంక్రమణ_అంచులు() { లేఖాచిత్రం_Alg_Remove_సంక్రమణ_అంచులు{ఇది}.go(); }

//######################################################################
//######################################################################
// Algorithms - weakly connected components
// Changes color()

తరగతి లేఖాచిత్రం_Alg_Weakly final : లేఖాచిత్రం_Alg<> {
    // Sentinel color, meaning not colored yet. Colors themselves are 0 .. m_numColors-1.
    స్థిర constexpr uint32_t UNCOLORED = std::numeric_limits<uint32_t>::max();

    uint32_t m_అంకె_Colors = 0;  // Number of colors assigned

    శూన్య ప్రధాన() {
        // Initialize state
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& శీర్షం : m_graphp->vertices()) శీర్షం.color(UNCOLORED);
        // Color graph, without gaps
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& శీర్షం : m_graphp->vertices()) {
            ఒకవేళ (శీర్షం.color() == UNCOLORED) శీర్షం_Iterate(&శీర్షం, m_అంకె_Colors++);
        }
    }

    శూన్య శీర్షం_Iterate(V3_లేఖాచిత్రం_శీర్షం* vertexp, uint32_t currentColor) {
        // Assign new color to each unvisited node
        // then visit each of its edges, giving them the same color
        ఒకవేళ (vertexp->color() != UNCOLORED) తిరిగి;  // Already colored it
        vertexp->color(currentColor);
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vertexp->బహిర్గతం_అంచులు()) {
            ఒకవేళ (follow_అంచు(&అంచు)) శీర్షం_Iterate(అంచు.శిఖరం(), currentColor);
        }
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vertexp->లోపల_అంచులు()) {
            ఒకవేళ (follow_అంచు(&అంచు)) శీర్షం_Iterate(అంచు.fromp(), currentColor);
        }
    }

బహిరంగ:
    లేఖాచిత్రం_Alg_Weakly(V3_లేఖాచిత్రం* graphp, V3_అంచు_Func_P అంచు_Funcp)
        : లేఖాచిత్రం_Alg<>(graphp, అంచు_Funcp) {
        ప్రధాన();
    }
    ~లేఖాచిత్రం_Alg_Weakly() = అప్రమేయ;

    uint32_t అంకె_Colors() స్థిరరాశి { తిరిగి m_అంకె_Colors; }
};

uint32_t V3_లేఖాచిత్రం::weaklyConnected(V3_అంచు_Func_P అంచు_Funcp) {
    తిరిగి లేఖాచిత్రం_Alg_Weakly{ఇది, అంచు_Funcp}.అంకె_Colors();
}

//######################################################################
//######################################################################
// Algorithms - strongly connected components
// Changes user() and color()

తరగతి లేఖాచిత్రం_Alg_Strongly final : లేఖాచిత్రం_Alg<> {
    uint32_t m_currentDfs = 0;  // DFS count
    std::సదిశ<V3_లేఖాచిత్రం_శీర్షం*> m_callTrace;  // List of everything we hit processing so far

    శూన్య ప్రధాన() {
        // Use Pearce's algorithm to color the strongly connected components. For reference see
        // "An Improved Algorithm for Finding the Strongly Connected Components of a Directed
        // Graph", David J.Pearce, 2005
        //
        // Node State:
        //     Vertex::user     // DFS number indicating possible root of subtree, 0=not iterated
        //     Vertex::color    // Output subtree number (fully processed)

        // Clear info
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& శీర్షం : m_graphp->vertices()) {
            శీర్షం.color(0);
            శీర్షం.user(0);
        }
        // Color graph
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& శీర్షం : m_graphp->vertices()) {
            ఒకవేళ (!శీర్షం.user()) {
                m_currentDfs++;
                శీర్షం_Iterate(&శీర్షం);
            }
        }
        // If there's a single vertex of a color, it doesn't need a subgraph
        // This simplifies the consumer's code, and reduces graph debugging clutter
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& శీర్షం : m_graphp->vertices()) {
            తార్కిక onecolor = నిజం;
            కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : శీర్షం.బహిర్గతం_అంచులు()) {
                ఒకవేళ (follow_అంచు(&అంచు)) {
                    ఒకవేళ (శీర్షం.color() == అంచు.శిఖరం()->color()) {
                        onecolor = అబద్ధం;
                        ఆపు;
                    }
                }
            }
            ఒకవేళ (onecolor) శీర్షం.color(0);
        }
    }

    శూన్య శీర్షం_Iterate(V3_లేఖాచిత్రం_శీర్షం* vertexp) {
        స్థిరరాశి uint32_t this_Dfs_అంకె = m_currentDfs++;
        vertexp->user(this_Dfs_అంకె);
        vertexp->color(0);
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vertexp->బహిర్గతం_అంచులు()) {
            ఒకవేళ (follow_అంచు(&అంచు)) {
                V3_లేఖాచిత్రం_శీర్షం* శిఖరం = అంచు.శిఖరం();
                ఒకవేళ (!శిఖరం->user()) {  // Dest not computed yet
                    శీర్షం_Iterate(శిఖరం);
                }
                ఒకవేళ (!శిఖరం->color()) {  // Dest not in a component
                    ఒకవేళ (vertexp->user() > శిఖరం->user()) vertexp->user(శిఖరం->user());
                }
            }
        }
        ఒకవేళ (vertexp->user() == this_Dfs_అంకె) {  // New head of subtree
            vertexp->color(this_Dfs_అంకె);  // Mark as component
            అయితే (!m_callTrace.ఖాళీలు()) {
                V3_లేఖాచిత్రం_శీర్షం* తీయి_Vertexp = m_callTrace.వెనుక();
                ఒకవేళ (తీయి_Vertexp->user() >= this_Dfs_అంకె) {  // Lower node is part of this subtree
                    m_callTrace.తీయి_వెనుక();
                    తీయి_Vertexp->color(this_Dfs_అంకె);
                } లేకపోతే {
                    ఆపు;
                }
            }
        } లేకపోతే {  // In another subtree (maybe...)
            m_callTrace.నెట్టు_వెనుక(vertexp);
        }
    }

బహిరంగ:
    లేఖాచిత్రం_Alg_Strongly(V3_లేఖాచిత్రం* graphp, V3_అంచు_Func_P అంచు_Funcp)
        : లేఖాచిత్రం_Alg<>{graphp, అంచు_Funcp} {
        ప్రధాన();
    }
    ~లేఖాచిత్రం_Alg_Strongly() = అప్రమేయ;
};

శూన్య V3_లేఖాచిత్రం::stronglyConnected(V3_అంచు_Func_P అంచు_Funcp) { లేఖాచిత్రం_Alg_Strongly{ఇది, అంచు_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - ranking
// Changes user() and rank()

తరగతి లేఖాచిత్రం_Alg_Rank final : లేఖాచిత్రం_Alg<> {
    శూన్య ప్రధాన() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& శీర్షం : m_graphp->vertices()) {
            శీర్షం.rank(0);
            శీర్షం.user(0);
        }
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& శీర్షం : m_graphp->vertices()) {
            ఒకవేళ (!శీర్షం.user()) శీర్షం_Iterate(&శీర్షం, 1);
        }
    }
    శూన్య శీర్షం_Iterate(V3_లేఖాచిత్రం_శీర్షం* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // If larger rank is found, assign it and loop back through
        // If we hit a back node make a list of all loops
        ఒకవేళ (vertexp->user() == 1) {
            m_graphp->loops_సందేశం_Cb(vertexp, m_అంచు_Funcp);
            తిరిగి;  // LCOV_EXCL_LINE  // gcc gprof bug misses this return
        }
        ఒకవేళ (vertexp->rank() >= currentRank) తిరిగి;  // Already processed it
        vertexp->user(1);
        vertexp->rank(currentRank);
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vertexp->బహిర్గతం_అంచులు()) {
            ఒకవేళ (follow_అంచు(&అంచు)) శీర్షం_Iterate(అంచు.శిఖరం(), currentRank + vertexp->rankAdder());
        }
        vertexp->user(2);
    }

బహిరంగ:
    లేఖాచిత్రం_Alg_Rank(V3_లేఖాచిత్రం* graphp, V3_అంచు_Func_P అంచు_Funcp)
        : లేఖాచిత్రం_Alg<>{graphp, అంచు_Funcp} {
        ప్రధాన();
    }
    ~లేఖాచిత్రం_Alg_Rank() = అప్రమేయ;
};

శూన్య V3_లేఖాచిత్రం::rank() { rank(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue); }
శూన్య V3_లేఖాచిత్రం::rank(V3_అంచు_Func_P అంచు_Funcp) { లేఖాచిత్రం_Alg_Rank{ఇది, అంచు_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - ranking min
// Changes user() and rank()

తరగతి లేఖాచిత్రం_Alg_Rank_Min final : లేఖాచిత్రం_Alg<> {
    శూన్య ప్రధాన() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& శీర్షం : m_graphp->vertices()) {
            శీర్షం.rank(0);
            శీర్షం.user(0);
        }
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& శీర్షం : m_graphp->vertices()) {
            ఒకవేళ (!శీర్షం.user()) శీర్షం_Iterate(&శీర్షం);
        }
    }
    uint32_t శీర్షం_Iterate(V3_లేఖాచిత్రం_శీర్షం* vertexp) {
        // Assign rank to each unvisited node
        // If we hit a back node make a list of all loops
        ఒకవేళ (vertexp->user() == 1) {
            m_graphp->loops_సందేశం_Cb(vertexp, m_అంచు_Funcp);
            తిరిగి vertexp->rank();
        }
        ఒకవేళ (vertexp->user()) తిరిగి vertexp->rank();  // Done earlier
        vertexp->user(1);
        vertexp->rank(1);  // In case loop
        // If no input edges, then rank 1 (+ adder)
        // Otherwise, get minimum from following all inputs.
        uint32_t minrank = ~0U;
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vertexp->లోపల_అంచులు()) {
            ఒకవేళ (follow_అంచు(&అంచు)) {
                స్థిరరాశి uint32_t nrank = శీర్షం_Iterate(అంచు.fromp());
                minrank = std::min(minrank, nrank);
            }
        }
        ఒకవేళ (minrank == ~0U) minrank = 0;
        vertexp->rank(minrank + vertexp->rankAdder());
        vertexp->user(2);
        తిరిగి vertexp->rank();
    }

బహిరంగ:
    లేఖాచిత్రం_Alg_Rank_Min(V3_లేఖాచిత్రం* graphp, V3_అంచు_Func_P అంచు_Funcp)
        : లేఖాచిత్రం_Alg<>{graphp, అంచు_Funcp} {
        ప్రధాన();
    }
    ~లేఖాచిత్రం_Alg_Rank_Min() = అప్రమేయ;
};

శూన్య V3_లేఖాచిత్రం::rankMin() { rankMin(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue); }
శూన్య V3_లేఖాచిత్రం::rankMin(V3_అంచు_Func_P అంచు_Funcp) { లేఖాచిత్రం_Alg_Rank_Min{ఇది, అంచు_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - report loops
// Changes user()

తరగతి లేఖాచిత్రం_Alg_RLoops final : లేఖాచిత్రం_Alg<> {
    std::సదిశ<V3_లేఖాచిత్రం_శీర్షం*> m_callTrace;  // List of everything we hit processing so far
    std::సదిశ<string> m_msgs;  // Output messages
    తార్కిక m_పూర్తి = అబద్ధం;  // Exit algorithm

    శూన్య ప్రధాన(V3_లేఖాచిత్రం_శీర్షం* vertexp) {
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        m_graphp->userClearVertices();
        m_callTrace.reserve(100);
        శీర్షం_Iterate(vertexp, 0);
    }

    శూన్య శీర్షం_Iterate(V3_లేఖాచిత్రం_శీర్షం* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // When we hit ourself again, return the list of all loops
        ఒకవేళ (m_పూర్తి) తిరిగి;

        // Can't just reserve(), unless we modify size() before setting array directly
        అయితే (m_callTrace.కొలత() <= currentRank) m_callTrace.నెట్టు_వెనుక(vertexp);
        m_callTrace[currentRank++] = vertexp;

        ఒకవేళ (vertexp->user() == 1) {
            కొరకు (గుర్తులేని i = 0; i < currentRank; i++)
                m_msgs.emplace_వెనుక(m_graphp->loops_శీర్షం_Cb(m_callTrace[i]));
            m_పూర్తి = నిజం;
            తిరిగి;
        }
        ఒకవేళ (vertexp->user() == 2) తిరిగి;  // Already processed it
        vertexp->user(1);
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vertexp->బహిర్గతం_అంచులు()) {
            ఒకవేళ (follow_అంచు(&అంచు)) శీర్షం_Iterate(అంచు.శిఖరం(), currentRank);
        }
        vertexp->user(2);
    }

బహిరంగ:
    లేఖాచిత్రం_Alg_RLoops(V3_లేఖాచిత్రం* graphp, V3_అంచు_Func_P అంచు_Funcp, V3_లేఖాచిత్రం_శీర్షం* vertexp)
        : లేఖాచిత్రం_Alg<>{graphp, అంచు_Funcp} {
        ప్రధాన(vertexp);
    }
    ~లేఖాచిత్రం_Alg_RLoops() = అప్రమేయ;
    string సందేశం() స్థిరరాశి {
        తిరిగి std::accumulate(m_msgs.begin(), m_msgs.చివర(), std::string{""});
    }
};

string V3_లేఖాచిత్రం::reportLoops(V3_అంచు_Func_P అంచు_Funcp, V3_లేఖాచిత్రం_శీర్షం* vertexp) {
    తిరిగి లేఖాచిత్రం_Alg_RLoops{ఇది, అంచు_Funcp, vertexp}.సందేశం();
}

//######################################################################
//######################################################################
// Algorithms - subtrees
// Changes user()

తరగతి లేఖాచిత్రం_Alg_Subtrees final : లేఖాచిత్రం_Alg<> {
    V3_లేఖాచిత్రం* స్థిరరాశి m_loopGraphp;

    //! Iterate through all connected nodes of a graph with a loop or loops.
    V3_లేఖాచిత్రం_శీర్షం* శీర్షం_Iterate_All(V3_లేఖాచిత్రం_శీర్షం* vertexp) {
        ఒకవేళ (V3_లేఖాచిత్రం_శీర్షం* newVertexp = static_cast<V3_లేఖాచిత్రం_శీర్షం*>(vertexp->userp())) {
            తిరిగి newVertexp;
        } లేకపోతే {
            newVertexp = vertexp->clone(m_loopGraphp);
            vertexp->userp(newVertexp);

            కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vertexp->బహిర్గతం_అంచులు()) {
                ఒకవేళ (follow_అంచు(&అంచు)) {
                    V3_లేఖాచిత్రం_అంచు* newEdgep = static_cast<V3_లేఖాచిత్రం_అంచు*>(అంచు.userp());
                    ఒకవేళ (!newEdgep) {
                        V3_లేఖాచిత్రం_శీర్షం* new_శిఖరం = శీర్షం_Iterate_All(అంచు.శిఖరం());
                        newEdgep = అంచు.clone(m_loopGraphp, newVertexp, new_శిఖరం);
                        అంచు.userp(newEdgep);
                    }
                }
            }
            తిరిగి newVertexp;
        }
    }

బహిరంగ:
    లేఖాచిత్రం_Alg_Subtrees(V3_లేఖాచిత్రం* graphp, V3_లేఖాచిత్రం* loopGraphp, V3_అంచు_Func_P అంచు_Funcp,
                     V3_లేఖాచిత్రం_శీర్షం* vertexp)
        : లేఖాచిత్రం_Alg<>{graphp, అంచు_Funcp}
        , m_loopGraphp{loopGraphp} {
        // Vertex::m_userp - New vertex if we have seen this vertex already
        // Edge::m_userp - New edge if we have seen this edge already
        m_graphp->userClearVertices();
        m_graphp->user_Clear_అంచులు();
        (శూన్య)శీర్షం_Iterate_All(vertexp);
    }
    ~లేఖాచిత్రం_Alg_Subtrees() = అప్రమేయ;
};

//! Report the entire connected graph with a loop or loops
శూన్య V3_లేఖాచిత్రం::subtreeLoops(V3_అంచు_Func_P అంచు_Funcp, V3_లేఖాచిత్రం_శీర్షం* vertexp, V3_లేఖాచిత్రం* loopGraphp) {
    లేఖాచిత్రం_Alg_Subtrees{ఇది, loopGraphp, అంచు_Funcp, vertexp};
}

//######################################################################
//######################################################################
// Algorithms - sorting

శూన్య V3_లేఖాచిత్రం::క్రమీకరించు_Vertices() {
    // Sort list of vertices by rank, then fanout
    std::సదిశ<V3_లేఖాచిత్రం_శీర్షం*> vertexps;
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& శీర్షం : m_vertices) vertexps.నెట్టు_వెనుక(&శీర్షం);
    std::stable_క్రమీకరించు(vertexps.begin(), vertexps.చివర(),
                     [](స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* lhsp, స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* rhsp) {  //
                         తిరిగి lhsp->క్రమీకరించు_Cmp(rhsp) < 0;
                     });
    // Re-insert in sorted order
    కొరకు (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vertexp : vertexps) {
        m_vertices.unlink(vertexp);
        m_vertices.link_వెనుక(vertexp);
    }
}

శూన్య V3_లేఖాచిత్రం::క్రమీకరించు_అంచులు() {
    // Sort edges by rank then fanout of node they point to
    std::సదిశ<V3_లేఖాచిత్రం_అంచు*> అంచులు;
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& శీర్షం : vertices()) {
        // Make a vector
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : శీర్షం.బహిర్గతం_అంచులు()) అంచులు.నెట్టు_వెనుక(&అంచు);
        //క్రమీకరించు
        std::stable_క్రమీకరించు(అంచులు.begin(), అంచులు.చివర(),
                         [](స్థిరరాశి V3_లేఖాచిత్రం_అంచు* lhsp, స్థిరరాశి V3_లేఖాచిత్రం_అంచు* rhsp) {  //
                             తిరిగి lhsp->క్రమీకరించు_Cmp(rhsp) < 0;
                         });
        // Relink edges in specified order
        కొరకు (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep : అంచులు) edgep->relinkFromp(&శీర్షం);
        // Prep for next
        అంచులు.clear();
    }
}

//######################################################################
//######################################################################
// Algorithms - ordering
//      Compute near optimal ordering of the nodes, where:
//          If a required    edge is A->B, rank(A)<rank(B)
//          Visit edges and assign ranks to keep minimal crossings
//              (Results in better dcache packing.)

శూన్య V3_లేఖాచిత్రం::క్రమం() {
    UINFO(2, "Order:");

    // Compute rankings again
    rank(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);
    క్రమం_Pre_Ranked();
}

శూన్య V3_లేఖాచిత్రం::క్రమం_Pre_Ranked() {
    // Compute fanouts
    // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
    userClearVertices();
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& శీర్షం : vertices()) {
        ఒకవేళ (!శీర్షం.user()) క్రమం_DFSIterate(&శీర్షం);
    }

    // Sort list of vertices by rank, then fanout. Fanout is a bit of a
    // misnomer. It is the sum of all the fanouts of nodes reached from a node
    // *plus* the count of edges in to that node.
    క్రమీకరించు_Vertices();
    // Sort edges by rank then fanout of node they point to
    క్రమీకరించు_అంచులు();
}

ద్విగుణ V3_లేఖాచిత్రం::క్రమం_DFSIterate(V3_లేఖాచిత్రం_శీర్షం* vertexp) {
    // Compute fanouts of each node
    // If forward edge, don't double count that fanout
    ఒకవేళ (vertexp->user() == 2) తిరిగి vertexp->fanout();  // Already processed it
    UASSERT_OBJ(vertexp->user() != 1, vertexp, "Loop found, backward edges should be dead");
    vertexp->user(1);
    ద్విగుణ fanout = 0;
    కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vertexp->బహిర్గతం_అంచులు()) {
        ఒకవేళ (అంచు.బరువు_2()) fanout += క్రమం_DFSIterate(అంచు.శిఖరం());
    }
    // Just count inbound edges
    కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vertexp->లోపల_అంచులు()) {
        ఒకవేళ (అంచు.బరువు_2()) ++fanout;
    }
    vertexp->fanout(fanout);
    vertexp->user(2);
    తిరిగి vertexp->fanout();
}

//######################################################################
//######################################################################
// Algorithms - parallelism report

తరగతి లేఖాచిత్రం_Alg_Parallelism_Report final {
    // MEMBERS
    V3_లేఖాచిత్రం& m_లేఖాచిత్రం;  // The graph
    స్థిరరాశి std::function<uint64_t(స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం*)> m_శీర్షం_ఖర్చు;  // vertex cost function
    V3_లేఖాచిత్రం::ParallelismReport m_report;  // The result report

    // CONSTRUCTORS
    స్పష్టమైన లేఖాచిత్రం_Alg_Parallelism_Report(V3_లేఖాచిత్రం& లేఖాచిత్రం,
                                       std::function<uint64_t(స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం*)> శీర్షం_ఖర్చు)
        : m_లేఖాచిత్రం{లేఖాచిత్రం}
        , m_శీర్షం_ఖర్చు{శీర్షం_ఖర్చు} {
        // For each node, record the critical path cost from the start
        // of the graph through the end of the node.
        std::unordered_map<స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం*, uint64_t> critPaths;
        లేఖాచిత్రం_ప్రవాహం_Unordered serialize{&m_లేఖాచిత్రం};
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* vertexp; (vertexp = serialize.nextp());) {
            ++m_report.m_శీర్షం_లెక్క;
            uint64_t cp_ఖర్చు_గమ్యం_ఇక్కడ = 0;
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : vertexp->లోపల_అంచులు()) {
                ++m_report.m_అంచు_లెక్క;
                // For each upstream item, add its critical path cost to
                // the cost of this edge, to form a new candidate critical
                // path cost to the current node. Whichever is largest is
                // the critical path to reach the start of this node.
                cp_ఖర్చు_గమ్యం_ఇక్కడ = std::max(cp_ఖర్చు_గమ్యం_ఇక్కడ, critPaths[అంచు.fromp()]);
            }
            // Include the cost of the current vertex in the critical
            // path, so it represents the critical path to the end of
            // this vertex.
            cp_ఖర్చు_గమ్యం_ఇక్కడ += m_శీర్షం_ఖర్చు(vertexp);
            critPaths[vertexp] = cp_ఖర్చు_గమ్యం_ఇక్కడ;
            m_report.m_critical_మార్గం_ఖర్చు = std::max(m_report.m_critical_మార్గం_ఖర్చు, cp_ఖర్చు_గమ్యం_ఇక్కడ);
            // Tally the total cost contributed by vertices.
            m_report.m_మొత్తం_లేఖాచిత్రం_ఖర్చు += m_శీర్షం_ఖర్చు(vertexp);
        }
    }
    ~లేఖాచిత్రం_Alg_Parallelism_Report() = అప్రమేయ;
    VL_UNCOPYABLE(లేఖాచిత్రం_Alg_Parallelism_Report);
    VL_UNMOVABLE(లేఖాచిత్రం_Alg_Parallelism_Report);

బహిరంగ:
    స్థిర V3_లేఖాచిత్రం::ParallelismReport
    apply(V3_లేఖాచిత్రం& లేఖాచిత్రం, std::function<uint32_t(స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం*)> శీర్షం_ఖర్చు) {
        తిరిగి లేఖాచిత్రం_Alg_Parallelism_Report(లేఖాచిత్రం, శీర్షం_ఖర్చు).m_report;
    }
};

V3_లేఖాచిత్రం::ParallelismReport
V3_లేఖాచిత్రం::parallelismReport(std::function<uint64_t(స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం*)> శీర్షం_ఖర్చు) {
    తిరిగి లేఖాచిత్రం_Alg_Parallelism_Report::apply(*ఇది, శీర్షం_ఖర్చు);
}
