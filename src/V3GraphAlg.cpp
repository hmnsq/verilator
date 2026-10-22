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

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_MT_DISABLED_ਕੋਡ_UNIT 1

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3GraphAlg.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3GraphPathChecker.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3GraphStream.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <limits>
#ਅਨ੍ਤਰ੍ਭਾਵ <list>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <numeric>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - Remove redundancies
// Changes user() and weight()

ਵਰਗ ਗ੍ਰਾਫ_Remove_Redundant final : ਗ੍ਰਾਫ_Alg<> {
    ਅਚਲ ਤਾਰਕਿਕ m_ਜੋੜ_Weights;  // Sum, rather then maximize weights
ਨਿਜੀ:
    ਖਾਲੀ ਮੁੱਖ() {
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : m_graphp->vertices()) ਸ਼ੀਰਸ਼_Iterate(&ਸ਼ੀਰਸ਼);
    }
    ਖਾਲੀ ਸ਼ੀਰਸ਼_Iterate(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp) {
        // Clear marks
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) ਕਿਨਾਰਾ.ਸਿਖਰ()->userp(ਖਾਲੀ_ਸੂਚਕ);

        // Mark edges and detect duplications

        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep : vertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().unlinkable()) {
            ਜੇ (follow_ਕਿਨਾਰਾ(edgep)) {
                V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਬਾਹਰਲਾ_Vertexp = edgep->ਸਿਖਰ();
                V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* prevEdgep = static_cast<V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ*>(ਬਾਹਰਲਾ_Vertexp->userp());
                ਜੇ (!prevEdgep) {  // No previous assignment
                    ਬਾਹਰਲਾ_Vertexp->userp(edgep);
                } ਨਹੀਂਤਾਂ {  // Duplicate
                    ਤਾਰਕਿਕ saveOld = ਸੱਚ;
                    ਜੇ (prevEdgep->cutable() && !edgep->cutable()) {
                        saveOld = ਝੂਠ;  // new !cutable more important than old
                    } ਨਹੀਂਤਾਂ ਜੇ (!prevEdgep->cutable() && edgep->cutable()) {
                        saveOld = ਸੱਚ;  // old !cutable more important than new
                    } ਨਹੀਂਤਾਂ {
                        saveOld = ਸੱਚ;
                        ਜੇ (!m_ਜੋੜ_Weights
                            && (prevEdgep->ਭਾਰ_2() < edgep->ਭਾਰ_2())) {  // Keep max weight
                            prevEdgep->ਭਾਰ_2(edgep->ਭਾਰ_2());
                        }
                    }
                    ਜੇ (saveOld) {
                        ਜੇ (m_ਜੋੜ_Weights) prevEdgep->ਭਾਰ_2(prevEdgep->ਭਾਰ_2() + edgep->ਭਾਰ_2());
                        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    } ਨਹੀਂਤਾਂ {
                        ਜੇ (m_ਜੋੜ_Weights) edgep->ਭਾਰ_2(prevEdgep->ਭਾਰ_2() + edgep->ਭਾਰ_2());
                        VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                        ਬਾਹਰਲਾ_Vertexp->userp(edgep);
                    }
                }
            }
        }
    }

ਜਨਤਕ:
    ਗ੍ਰਾਫ_Remove_Redundant(V3_ਗ੍ਰਾਫ* graphp, V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp, ਤਾਰਕਿਕ ਜੋੜ_Weights)
        : ਗ੍ਰਾਫ_Alg<>{graphp, ਕਿਨਾਰਾ_Funcp}
        , m_ਜੋੜ_Weights{ਜੋੜ_Weights} {
        ਮੁੱਖ();
    }
    ~ਗ੍ਰਾਫ_Remove_Redundant() = ਮੂਲ;
};

ਖਾਲੀ V3_ਗ੍ਰਾਫ::remove_Redundant_ਕਿਨਾਰੇ_Max(V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp) {
    ਗ੍ਰਾਫ_Remove_Redundant{ਇਹ, ਕਿਨਾਰਾ_Funcp, ਝੂਠ};
}
ਖਾਲੀ V3_ਗ੍ਰਾਫ::remove_Redundant_ਕਿਨਾਰੇ_ਜੋੜ(V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp) {
    ਗ੍ਰਾਫ_Remove_Redundant{ਇਹ, ਕਿਨਾਰਾ_Funcp, ਸੱਚ};
}

//######################################################################
//######################################################################
// Algorithms - remove transitive

ਵਰਗ ਗ੍ਰਾਫ_Alg_Remove_ਸੰਕ੍ਰਾਮਕ_ਕਿਨਾਰੇ final : ਗ੍ਰਾਫ_Alg<> {
ਜਨਤਕ:
    ਸਪਸ਼ਟ ਗ੍ਰਾਫ_Alg_Remove_ਸੰਕ੍ਰਾਮਕ_ਕਿਨਾਰੇ(V3_ਗ੍ਰਾਫ* graphp)
        : ਗ੍ਰਾਫ_Alg<>(graphp, ਖਾਲੀ_ਸੂਚਕ) {}
    ਖਾਲੀ go() {
        ਗ੍ਰਾਫ_ਰਸਤਾ_Checker checker{m_graphp};
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_graphp->vertices()) {
            V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* deletep = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtx.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                ਜੇ (deletep) VL_DO_CLEAR(deletep->unlinkDelete(), deletep = ਖਾਲੀ_ਸੂਚਕ);
                // It should be safe to modify the graph, despite using
                // the GraphPathChecker, as none of the modifications will
                // change what can be reached from what, nor should they
                // change the rank or CP of any node.
                ਜੇ (checker.is_ਸੰਕ੍ਰਾਮਕ_ਕਿਨਾਰਾ(&ਕਿਨਾਰਾ)) deletep = &ਕਿਨਾਰਾ;
            }
            ਜੇ (deletep) VL_DO_DANGLING(deletep->unlinkDelete(), deletep);
        }
    }

ਨਿਜੀ:
    VL_UNCOPYABLE(ਗ੍ਰਾਫ_Alg_Remove_ਸੰਕ੍ਰਾਮਕ_ਕਿਨਾਰੇ);
};

ਖਾਲੀ V3_ਗ੍ਰਾਫ::remove_ਸੰਕ੍ਰਾਮਕ_ਕਿਨਾਰੇ() { ਗ੍ਰਾਫ_Alg_Remove_ਸੰਕ੍ਰਾਮਕ_ਕਿਨਾਰੇ{ਇਹ}.go(); }

//######################################################################
//######################################################################
// Algorithms - weakly connected components
// Changes color()

ਵਰਗ ਗ੍ਰਾਫ_Alg_Weakly final : ਗ੍ਰਾਫ_Alg<> {
    // Sentinel color, meaning not colored yet. Colors themselves are 0 .. m_numColors-1.
    ਸਥਿਰ constexpr uint32_t UNCOLORED = std::numeric_limits<uint32_t>::max();

    uint32_t m_ਅੰਕ_Colors = 0;  // Number of colors assigned

    ਖਾਲੀ ਮੁੱਖ() {
        // Initialize state
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : m_graphp->vertices()) ਸ਼ੀਰਸ਼.color(UNCOLORED);
        // Color graph, without gaps
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : m_graphp->vertices()) {
            ਜੇ (ਸ਼ੀਰਸ਼.color() == UNCOLORED) ਸ਼ੀਰਸ਼_Iterate(&ਸ਼ੀਰਸ਼, m_ਅੰਕ_Colors++);
        }
    }

    ਖਾਲੀ ਸ਼ੀਰਸ਼_Iterate(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp, uint32_t currentColor) {
        // Assign new color to each unvisited node
        // then visit each of its edges, giving them the same color
        ਜੇ (vertexp->color() != UNCOLORED) ਵਾਪਸ;  // Already colored it
        vertexp->color(currentColor);
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            ਜੇ (follow_ਕਿਨਾਰਾ(&ਕਿਨਾਰਾ)) ਸ਼ੀਰਸ਼_Iterate(ਕਿਨਾਰਾ.ਸਿਖਰ(), currentColor);
        }
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vertexp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
            ਜੇ (follow_ਕਿਨਾਰਾ(&ਕਿਨਾਰਾ)) ਸ਼ੀਰਸ਼_Iterate(ਕਿਨਾਰਾ.fromp(), currentColor);
        }
    }

ਜਨਤਕ:
    ਗ੍ਰਾਫ_Alg_Weakly(V3_ਗ੍ਰਾਫ* graphp, V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp)
        : ਗ੍ਰਾਫ_Alg<>(graphp, ਕਿਨਾਰਾ_Funcp) {
        ਮੁੱਖ();
    }
    ~ਗ੍ਰਾਫ_Alg_Weakly() = ਮੂਲ;

    uint32_t ਅੰਕ_Colors() ਅਚਲ { ਵਾਪਸ m_ਅੰਕ_Colors; }
};

uint32_t V3_ਗ੍ਰਾਫ::weaklyConnected(V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp) {
    ਵਾਪਸ ਗ੍ਰਾਫ_Alg_Weakly{ਇਹ, ਕਿਨਾਰਾ_Funcp}.ਅੰਕ_Colors();
}

//######################################################################
//######################################################################
// Algorithms - strongly connected components
// Changes user() and color()

ਵਰਗ ਗ੍ਰਾਫ_Alg_Strongly final : ਗ੍ਰਾਫ_Alg<> {
    uint32_t m_currentDfs = 0;  // DFS count
    std::ਸਦਿਸ਼<V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*> m_callTrace;  // List of everything we hit processing so far

    ਖਾਲੀ ਮੁੱਖ() {
        // Use Pearce's algorithm to color the strongly connected components. For reference see
        // "An Improved Algorithm for Finding the Strongly Connected Components of a Directed
        // Graph", David J.Pearce, 2005
        //
        // Node State:
        //     Vertex::user     // DFS number indicating possible root of subtree, 0=not iterated
        //     Vertex::color    // Output subtree number (fully processed)

        // Clear info
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : m_graphp->vertices()) {
            ਸ਼ੀਰਸ਼.color(0);
            ਸ਼ੀਰਸ਼.user(0);
        }
        // Color graph
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : m_graphp->vertices()) {
            ਜੇ (!ਸ਼ੀਰਸ਼.user()) {
                m_currentDfs++;
                ਸ਼ੀਰਸ਼_Iterate(&ਸ਼ੀਰਸ਼);
            }
        }
        // If there's a single vertex of a color, it doesn't need a subgraph
        // This simplifies the consumer's code, and reduces graph debugging clutter
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : m_graphp->vertices()) {
            ਤਾਰਕਿਕ onecolor = ਸੱਚ;
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : ਸ਼ੀਰਸ਼.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                ਜੇ (follow_ਕਿਨਾਰਾ(&ਕਿਨਾਰਾ)) {
                    ਜੇ (ਸ਼ੀਰਸ਼.color() == ਕਿਨਾਰਾ.ਸਿਖਰ()->color()) {
                        onecolor = ਝੂਠ;
                        ਰੋਕੋ;
                    }
                }
            }
            ਜੇ (onecolor) ਸ਼ੀਰਸ਼.color(0);
        }
    }

    ਖਾਲੀ ਸ਼ੀਰਸ਼_Iterate(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp) {
        ਅਚਲ uint32_t this_Dfs_ਅੰਕ = m_currentDfs++;
        vertexp->user(this_Dfs_ਅੰਕ);
        vertexp->color(0);
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            ਜੇ (follow_ਕਿਨਾਰਾ(&ਕਿਨਾਰਾ)) {
                V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ = ਕਿਨਾਰਾ.ਸਿਖਰ();
                ਜੇ (!ਸਿਖਰ->user()) {  // Dest not computed yet
                    ਸ਼ੀਰਸ਼_Iterate(ਸਿਖਰ);
                }
                ਜੇ (!ਸਿਖਰ->color()) {  // Dest not in a component
                    ਜੇ (vertexp->user() > ਸਿਖਰ->user()) vertexp->user(ਸਿਖਰ->user());
                }
            }
        }
        ਜੇ (vertexp->user() == this_Dfs_ਅੰਕ) {  // New head of subtree
            vertexp->color(this_Dfs_ਅੰਕ);  // Mark as component
            ਜਦੋਂ (!m_callTrace.ਖਾਲੀ_ਥਾਵਾਂ()) {
                V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਕੱਢੋ_Vertexp = m_callTrace.ਪਿੱਠ();
                ਜੇ (ਕੱਢੋ_Vertexp->user() >= this_Dfs_ਅੰਕ) {  // Lower node is part of this subtree
                    m_callTrace.ਕੱਢੋ_ਪਿੱਠ();
                    ਕੱਢੋ_Vertexp->color(this_Dfs_ਅੰਕ);
                } ਨਹੀਂਤਾਂ {
                    ਰੋਕੋ;
                }
            }
        } ਨਹੀਂਤਾਂ {  // In another subtree (maybe...)
            m_callTrace.ਧੱਕੋ_ਪਿੱਠ(vertexp);
        }
    }

ਜਨਤਕ:
    ਗ੍ਰਾਫ_Alg_Strongly(V3_ਗ੍ਰਾਫ* graphp, V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp)
        : ਗ੍ਰਾਫ_Alg<>{graphp, ਕਿਨਾਰਾ_Funcp} {
        ਮੁੱਖ();
    }
    ~ਗ੍ਰਾਫ_Alg_Strongly() = ਮੂਲ;
};

ਖਾਲੀ V3_ਗ੍ਰਾਫ::stronglyConnected(V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp) { ਗ੍ਰਾਫ_Alg_Strongly{ਇਹ, ਕਿਨਾਰਾ_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - ranking
// Changes user() and rank()

ਵਰਗ ਗ੍ਰਾਫ_Alg_Rank final : ਗ੍ਰਾਫ_Alg<> {
    ਖਾਲੀ ਮੁੱਖ() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : m_graphp->vertices()) {
            ਸ਼ੀਰਸ਼.rank(0);
            ਸ਼ੀਰਸ਼.user(0);
        }
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : m_graphp->vertices()) {
            ਜੇ (!ਸ਼ੀਰਸ਼.user()) ਸ਼ੀਰਸ਼_Iterate(&ਸ਼ੀਰਸ਼, 1);
        }
    }
    ਖਾਲੀ ਸ਼ੀਰਸ਼_Iterate(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // If larger rank is found, assign it and loop back through
        // If we hit a back node make a list of all loops
        ਜੇ (vertexp->user() == 1) {
            m_graphp->loops_ਸੁਨੇਹਾ_Cb(vertexp, m_ਕਿਨਾਰਾ_Funcp);
            ਵਾਪਸ;  // LCOV_EXCL_LINE  // gcc gprof bug misses this return
        }
        ਜੇ (vertexp->rank() >= currentRank) ਵਾਪਸ;  // Already processed it
        vertexp->user(1);
        vertexp->rank(currentRank);
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            ਜੇ (follow_ਕਿਨਾਰਾ(&ਕਿਨਾਰਾ)) ਸ਼ੀਰਸ਼_Iterate(ਕਿਨਾਰਾ.ਸਿਖਰ(), currentRank + vertexp->rankAdder());
        }
        vertexp->user(2);
    }

ਜਨਤਕ:
    ਗ੍ਰਾਫ_Alg_Rank(V3_ਗ੍ਰਾਫ* graphp, V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp)
        : ਗ੍ਰਾਫ_Alg<>{graphp, ਕਿਨਾਰਾ_Funcp} {
        ਮੁੱਖ();
    }
    ~ਗ੍ਰਾਫ_Alg_Rank() = ਮੂਲ;
};

ਖਾਲੀ V3_ਗ੍ਰਾਫ::rank() { rank(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue); }
ਖਾਲੀ V3_ਗ੍ਰਾਫ::rank(V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp) { ਗ੍ਰਾਫ_Alg_Rank{ਇਹ, ਕਿਨਾਰਾ_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - ranking min
// Changes user() and rank()

ਵਰਗ ਗ੍ਰਾਫ_Alg_Rank_Min final : ਗ੍ਰਾਫ_Alg<> {
    ਖਾਲੀ ਮੁੱਖ() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : m_graphp->vertices()) {
            ਸ਼ੀਰਸ਼.rank(0);
            ਸ਼ੀਰਸ਼.user(0);
        }
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : m_graphp->vertices()) {
            ਜੇ (!ਸ਼ੀਰਸ਼.user()) ਸ਼ੀਰਸ਼_Iterate(&ਸ਼ੀਰਸ਼);
        }
    }
    uint32_t ਸ਼ੀਰਸ਼_Iterate(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp) {
        // Assign rank to each unvisited node
        // If we hit a back node make a list of all loops
        ਜੇ (vertexp->user() == 1) {
            m_graphp->loops_ਸੁਨੇਹਾ_Cb(vertexp, m_ਕਿਨਾਰਾ_Funcp);
            ਵਾਪਸ vertexp->rank();
        }
        ਜੇ (vertexp->user()) ਵਾਪਸ vertexp->rank();  // Done earlier
        vertexp->user(1);
        vertexp->rank(1);  // In case loop
        // If no input edges, then rank 1 (+ adder)
        // Otherwise, get minimum from following all inputs.
        uint32_t minrank = ~0U;
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vertexp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
            ਜੇ (follow_ਕਿਨਾਰਾ(&ਕਿਨਾਰਾ)) {
                ਅਚਲ uint32_t nrank = ਸ਼ੀਰਸ਼_Iterate(ਕਿਨਾਰਾ.fromp());
                minrank = std::min(minrank, nrank);
            }
        }
        ਜੇ (minrank == ~0U) minrank = 0;
        vertexp->rank(minrank + vertexp->rankAdder());
        vertexp->user(2);
        ਵਾਪਸ vertexp->rank();
    }

ਜਨਤਕ:
    ਗ੍ਰਾਫ_Alg_Rank_Min(V3_ਗ੍ਰਾਫ* graphp, V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp)
        : ਗ੍ਰਾਫ_Alg<>{graphp, ਕਿਨਾਰਾ_Funcp} {
        ਮੁੱਖ();
    }
    ~ਗ੍ਰਾਫ_Alg_Rank_Min() = ਮੂਲ;
};

ਖਾਲੀ V3_ਗ੍ਰਾਫ::rankMin() { rankMin(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue); }
ਖਾਲੀ V3_ਗ੍ਰਾਫ::rankMin(V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp) { ਗ੍ਰਾਫ_Alg_Rank_Min{ਇਹ, ਕਿਨਾਰਾ_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - report loops
// Changes user()

ਵਰਗ ਗ੍ਰਾਫ_Alg_RLoops final : ਗ੍ਰਾਫ_Alg<> {
    std::ਸਦਿਸ਼<V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*> m_callTrace;  // List of everything we hit processing so far
    std::ਸਦਿਸ਼<string> m_msgs;  // Output messages
    ਤਾਰਕਿਕ m_ਪੂਰਾ = ਝੂਠ;  // Exit algorithm

    ਖਾਲੀ ਮੁੱਖ(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp) {
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        m_graphp->userClearVertices();
        m_callTrace.reserve(100);
        ਸ਼ੀਰਸ਼_Iterate(vertexp, 0);
    }

    ਖਾਲੀ ਸ਼ੀਰਸ਼_Iterate(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // When we hit ourself again, return the list of all loops
        ਜੇ (m_ਪੂਰਾ) ਵਾਪਸ;

        // Can't just reserve(), unless we modify size() before setting array directly
        ਜਦੋਂ (m_callTrace.ਮਾਪ() <= currentRank) m_callTrace.ਧੱਕੋ_ਪਿੱਠ(vertexp);
        m_callTrace[currentRank++] = vertexp;

        ਜੇ (vertexp->user() == 1) {
            ਲਈ (ਬਿਨਾਨਿਸ਼ਾਨੀ i = 0; i < currentRank; i++)
                m_msgs.emplace_ਪਿੱਠ(m_graphp->loops_ਸ਼ੀਰਸ਼_Cb(m_callTrace[i]));
            m_ਪੂਰਾ = ਸੱਚ;
            ਵਾਪਸ;
        }
        ਜੇ (vertexp->user() == 2) ਵਾਪਸ;  // Already processed it
        vertexp->user(1);
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            ਜੇ (follow_ਕਿਨਾਰਾ(&ਕਿਨਾਰਾ)) ਸ਼ੀਰਸ਼_Iterate(ਕਿਨਾਰਾ.ਸਿਖਰ(), currentRank);
        }
        vertexp->user(2);
    }

ਜਨਤਕ:
    ਗ੍ਰਾਫ_Alg_RLoops(V3_ਗ੍ਰਾਫ* graphp, V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp)
        : ਗ੍ਰਾਫ_Alg<>{graphp, ਕਿਨਾਰਾ_Funcp} {
        ਮੁੱਖ(vertexp);
    }
    ~ਗ੍ਰਾਫ_Alg_RLoops() = ਮੂਲ;
    string ਸੁਨੇਹਾ() ਅਚਲ {
        ਵਾਪਸ std::accumulate(m_msgs.begin(), m_msgs.ਸਿਰਾ(), std::string{""});
    }
};

string V3_ਗ੍ਰਾਫ::reportLoops(V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp) {
    ਵਾਪਸ ਗ੍ਰਾਫ_Alg_RLoops{ਇਹ, ਕਿਨਾਰਾ_Funcp, vertexp}.ਸੁਨੇਹਾ();
}

//######################################################################
//######################################################################
// Algorithms - subtrees
// Changes user()

ਵਰਗ ਗ੍ਰਾਫ_Alg_Subtrees final : ਗ੍ਰਾਫ_Alg<> {
    V3_ਗ੍ਰਾਫ* ਅਚਲ m_loopGraphp;

    //! Iterate through all connected nodes of a graph with a loop or loops.
    V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸ਼ੀਰਸ਼_Iterate_All(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp) {
        ਜੇ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* newVertexp = static_cast<V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*>(vertexp->userp())) {
            ਵਾਪਸ newVertexp;
        } ਨਹੀਂਤਾਂ {
            newVertexp = vertexp->clone(m_loopGraphp);
            vertexp->userp(newVertexp);

            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                ਜੇ (follow_ਕਿਨਾਰਾ(&ਕਿਨਾਰਾ)) {
                    V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* newEdgep = static_cast<V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ*>(ਕਿਨਾਰਾ.userp());
                    ਜੇ (!newEdgep) {
                        V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* new_ਸਿਖਰ = ਸ਼ੀਰਸ਼_Iterate_All(ਕਿਨਾਰਾ.ਸਿਖਰ());
                        newEdgep = ਕਿਨਾਰਾ.clone(m_loopGraphp, newVertexp, new_ਸਿਖਰ);
                        ਕਿਨਾਰਾ.userp(newEdgep);
                    }
                }
            }
            ਵਾਪਸ newVertexp;
        }
    }

ਜਨਤਕ:
    ਗ੍ਰਾਫ_Alg_Subtrees(V3_ਗ੍ਰਾਫ* graphp, V3_ਗ੍ਰਾਫ* loopGraphp, V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp,
                     V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp)
        : ਗ੍ਰਾਫ_Alg<>{graphp, ਕਿਨਾਰਾ_Funcp}
        , m_loopGraphp{loopGraphp} {
        // Vertex::m_userp - New vertex if we have seen this vertex already
        // Edge::m_userp - New edge if we have seen this edge already
        m_graphp->userClearVertices();
        m_graphp->user_Clear_ਕਿਨਾਰੇ();
        (ਖਾਲੀ)ਸ਼ੀਰਸ਼_Iterate_All(vertexp);
    }
    ~ਗ੍ਰਾਫ_Alg_Subtrees() = ਮੂਲ;
};

//! Report the entire connected graph with a loop or loops
ਖਾਲੀ V3_ਗ੍ਰਾਫ::subtreeLoops(V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp, V3_ਗ੍ਰਾਫ* loopGraphp) {
    ਗ੍ਰਾਫ_Alg_Subtrees{ਇਹ, loopGraphp, ਕਿਨਾਰਾ_Funcp, vertexp};
}

//######################################################################
//######################################################################
// Algorithms - sorting

ਖਾਲੀ V3_ਗ੍ਰਾਫ::ਛਾਂਟੋ_Vertices() {
    // Sort list of vertices by rank, then fanout
    std::ਸਦਿਸ਼<V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*> vertexps;
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : m_vertices) vertexps.ਧੱਕੋ_ਪਿੱਠ(&ਸ਼ੀਰਸ਼);
    std::stable_ਛਾਂਟੋ(vertexps.begin(), vertexps.ਸਿਰਾ(),
                     [](ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* rhsp) {  //
                         ਵਾਪਸ lhsp->ਛਾਂਟੋ_Cmp(rhsp) < 0;
                     });
    // Re-insert in sorted order
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vertexp : vertexps) {
        m_vertices.unlink(vertexp);
        m_vertices.link_ਪਿੱਠ(vertexp);
    }
}

ਖਾਲੀ V3_ਗ੍ਰਾਫ::ਛਾਂਟੋ_ਕਿਨਾਰੇ() {
    // Sort edges by rank then fanout of node they point to
    std::ਸਦਿਸ਼<V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ*> ਕਿਨਾਰੇ;
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : vertices()) {
        // Make a vector
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : ਸ਼ੀਰਸ਼.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) ਕਿਨਾਰੇ.ਧੱਕੋ_ਪਿੱਠ(&ਕਿਨਾਰਾ);
        //ਛਾਂਟੋ
        std::stable_ਛਾਂਟੋ(ਕਿਨਾਰੇ.begin(), ਕਿਨਾਰੇ.ਸਿਰਾ(),
                         [](ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* lhsp, ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* rhsp) {  //
                             ਵਾਪਸ lhsp->ਛਾਂਟੋ_Cmp(rhsp) < 0;
                         });
        // Relink edges in specified order
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep : ਕਿਨਾਰੇ) edgep->relinkFromp(&ਸ਼ੀਰਸ਼);
        // Prep for next
        ਕਿਨਾਰੇ.clear();
    }
}

//######################################################################
//######################################################################
// Algorithms - ordering
//      Compute near optimal ordering of the nodes, where:
//          If a required    edge is A->B, rank(A)<rank(B)
//          Visit edges and assign ranks to keep minimal crossings
//              (Results in better dcache packing.)

ਖਾਲੀ V3_ਗ੍ਰਾਫ::ਕ੍ਰਮ() {
    UINFO(2, "Order:");

    // Compute rankings again
    rank(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);
    ਕ੍ਰਮ_Pre_Ranked();
}

ਖਾਲੀ V3_ਗ੍ਰਾਫ::ਕ੍ਰਮ_Pre_Ranked() {
    // Compute fanouts
    // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
    userClearVertices();
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : vertices()) {
        ਜੇ (!ਸ਼ੀਰਸ਼.user()) ਕ੍ਰਮ_DFSIterate(&ਸ਼ੀਰਸ਼);
    }

    // Sort list of vertices by rank, then fanout. Fanout is a bit of a
    // misnomer. It is the sum of all the fanouts of nodes reached from a node
    // *plus* the count of edges in to that node.
    ਛਾਂਟੋ_Vertices();
    // Sort edges by rank then fanout of node they point to
    ਛਾਂਟੋ_ਕਿਨਾਰੇ();
}

ਦੁੱਗਣਾ V3_ਗ੍ਰਾਫ::ਕ੍ਰਮ_DFSIterate(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp) {
    // Compute fanouts of each node
    // If forward edge, don't double count that fanout
    ਜੇ (vertexp->user() == 2) ਵਾਪਸ vertexp->fanout();  // Already processed it
    UASSERT_OBJ(vertexp->user() != 1, vertexp, "Loop found, backward edges should be dead");
    vertexp->user(1);
    ਦੁੱਗਣਾ fanout = 0;
    ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
        ਜੇ (ਕਿਨਾਰਾ.ਭਾਰ_2()) fanout += ਕ੍ਰਮ_DFSIterate(ਕਿਨਾਰਾ.ਸਿਖਰ());
    }
    // Just count inbound edges
    ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vertexp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
        ਜੇ (ਕਿਨਾਰਾ.ਭਾਰ_2()) ++fanout;
    }
    vertexp->fanout(fanout);
    vertexp->user(2);
    ਵਾਪਸ vertexp->fanout();
}

//######################################################################
//######################################################################
// Algorithms - parallelism report

ਵਰਗ ਗ੍ਰਾਫ_Alg_Parallelism_Report final {
    // MEMBERS
    V3_ਗ੍ਰਾਫ& m_ਗ੍ਰਾਫ;  // The graph
    ਅਚਲ std::function<uint64_t(ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*)> m_ਸ਼ੀਰਸ਼_ਲਾਗਤ;  // vertex cost function
    V3_ਗ੍ਰਾਫ::ParallelismReport m_report;  // The result report

    // CONSTRUCTORS
    ਸਪਸ਼ਟ ਗ੍ਰਾਫ_Alg_Parallelism_Report(V3_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ,
                                       std::function<uint64_t(ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*)> ਸ਼ੀਰਸ਼_ਲਾਗਤ)
        : m_ਗ੍ਰਾਫ{ਗ੍ਰਾਫ}
        , m_ਸ਼ੀਰਸ਼_ਲਾਗਤ{ਸ਼ੀਰਸ਼_ਲਾਗਤ} {
        // For each node, record the critical path cost from the start
        // of the graph through the end of the node.
        std::unordered_map<ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*, uint64_t> critPaths;
        ਗ੍ਰਾਫ_ਧਾਰਾ_Unordered serialize{&m_ਗ੍ਰਾਫ};
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp; (vertexp = serialize.nextp());) {
            ++m_report.m_ਸ਼ੀਰਸ਼_ਗਿਣਤੀ;
            uint64_t cp_ਲਾਗਤ_ਮੰਜ਼ਿਲ_ਇੱਥੇ = 0;
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vertexp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
                ++m_report.m_ਕਿਨਾਰਾ_ਗਿਣਤੀ;
                // For each upstream item, add its critical path cost to
                // the cost of this edge, to form a new candidate critical
                // path cost to the current node. Whichever is largest is
                // the critical path to reach the start of this node.
                cp_ਲਾਗਤ_ਮੰਜ਼ਿਲ_ਇੱਥੇ = std::max(cp_ਲਾਗਤ_ਮੰਜ਼ਿਲ_ਇੱਥੇ, critPaths[ਕਿਨਾਰਾ.fromp()]);
            }
            // Include the cost of the current vertex in the critical
            // path, so it represents the critical path to the end of
            // this vertex.
            cp_ਲਾਗਤ_ਮੰਜ਼ਿਲ_ਇੱਥੇ += m_ਸ਼ੀਰਸ਼_ਲਾਗਤ(vertexp);
            critPaths[vertexp] = cp_ਲਾਗਤ_ਮੰਜ਼ਿਲ_ਇੱਥੇ;
            m_report.m_critical_ਰਸਤਾ_ਲਾਗਤ = std::max(m_report.m_critical_ਰਸਤਾ_ਲਾਗਤ, cp_ਲਾਗਤ_ਮੰਜ਼ਿਲ_ਇੱਥੇ);
            // Tally the total cost contributed by vertices.
            m_report.m_ਕੁੱਲ_ਗ੍ਰਾਫ_ਲਾਗਤ += m_ਸ਼ੀਰਸ਼_ਲਾਗਤ(vertexp);
        }
    }
    ~ਗ੍ਰਾਫ_Alg_Parallelism_Report() = ਮੂਲ;
    VL_UNCOPYABLE(ਗ੍ਰਾਫ_Alg_Parallelism_Report);
    VL_UNMOVABLE(ਗ੍ਰਾਫ_Alg_Parallelism_Report);

ਜਨਤਕ:
    ਸਥਿਰ V3_ਗ੍ਰਾਫ::ParallelismReport
    apply(V3_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ, std::function<uint32_t(ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*)> ਸ਼ੀਰਸ਼_ਲਾਗਤ) {
        ਵਾਪਸ ਗ੍ਰਾਫ_Alg_Parallelism_Report(ਗ੍ਰਾਫ, ਸ਼ੀਰਸ਼_ਲਾਗਤ).m_report;
    }
};

V3_ਗ੍ਰਾਫ::ParallelismReport
V3_ਗ੍ਰਾਫ::parallelismReport(std::function<uint64_t(ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*)> ਸ਼ੀਰਸ਼_ਲਾਗਤ) {
    ਵਾਪਸ ਗ੍ਰਾਫ_Alg_Parallelism_Report::apply(*ਇਹ, ਸ਼ੀਰਸ਼_ਲਾਗਤ);
}
