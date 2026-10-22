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

#ವ್ಯಾಖ್ಯಾನಿಸು VL_MT_DISABLED_ಸಂಕೇತ_UNIT 1

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3GraphAlg.h"

#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3GraphPathChecker.h"
#ಅನ್ತರ್ಭಾವ "V3GraphStream.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <limits>
#ಅನ್ತರ್ಭಾವ <list>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <numeric>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - Remove redundancies
// Changes user() and weight()

ವರ್ಗ ನಕ್ಷೆ_Remove_Redundant final : ನಕ್ಷೆ_Alg<> {
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_ಮೊತ್ತ_Weights;  // Sum, rather then maximize weights
ಖಾಸಗಿ:
    ಶೂನ್ಯ ಮುಖ್ಯ() {
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : m_graphp->vertices()) ಶೃಂಗ_Iterate(&ಶೃಂಗ);
    }
    ಶೂನ್ಯ ಶೃಂಗ_Iterate(V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp) {
        // Clear marks
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) ಅಂಚು.ತುದಿ()->userp(ಶೂನ್ಯಸೂಚಕ);

        // Mark edges and detect duplications

        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep : vertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().unlinkable()) {
            ಆದರೆ (follow_ಅಂಚು(edgep)) {
                V3_ನಕ್ಷೆ_ಶೃಂಗ* ಹೊರಹೋಗು_Vertexp = edgep->ತುದಿ();
                V3_ನಕ್ಷೆ_ಅಂಚು* prevEdgep = static_cast<V3_ನಕ್ಷೆ_ಅಂಚು*>(ಹೊರಹೋಗು_Vertexp->userp());
                ಆದರೆ (!prevEdgep) {  // No previous assignment
                    ಹೊರಹೋಗು_Vertexp->userp(edgep);
                } ಇಲ್ಲದಿದ್ದರೆ {  // Duplicate
                    ತಾರ್ಕಿಕ saveOld = ನಿಜ;
                    ಆದರೆ (prevEdgep->cutable() && !edgep->cutable()) {
                        saveOld = ಸುಳ್ಳು;  // new !cutable more important than old
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!prevEdgep->cutable() && edgep->cutable()) {
                        saveOld = ನಿಜ;  // old !cutable more important than new
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        saveOld = ನಿಜ;
                        ಆದರೆ (!m_ಮೊತ್ತ_Weights
                            && (prevEdgep->ತೂಕ_2() < edgep->ತೂಕ_2())) {  // Keep max weight
                            prevEdgep->ತೂಕ_2(edgep->ತೂಕ_2());
                        }
                    }
                    ಆದರೆ (saveOld) {
                        ಆದರೆ (m_ಮೊತ್ತ_Weights) prevEdgep->ತೂಕ_2(prevEdgep->ತೂಕ_2() + edgep->ತೂಕ_2());
                        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        ಆದರೆ (m_ಮೊತ್ತ_Weights) edgep->ತೂಕ_2(prevEdgep->ತೂಕ_2() + edgep->ತೂಕ_2());
                        VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                        ಹೊರಹೋಗು_Vertexp->userp(edgep);
                    }
                }
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    ನಕ್ಷೆ_Remove_Redundant(V3_ನಕ್ಷೆ* graphp, V3_ಅಂಚು_Func_P ಅಂಚು_Funcp, ತಾರ್ಕಿಕ ಮೊತ್ತ_Weights)
        : ನಕ್ಷೆ_Alg<>{graphp, ಅಂಚು_Funcp}
        , m_ಮೊತ್ತ_Weights{ಮೊತ್ತ_Weights} {
        ಮುಖ್ಯ();
    }
    ~ನಕ್ಷೆ_Remove_Redundant() = ಪೂರ್ವನಿಯೋಜಿತ;
};

ಶೂನ್ಯ V3_ನಕ್ಷೆ::remove_Redundant_ಅಂಚುಗಳು_Max(V3_ಅಂಚು_Func_P ಅಂಚು_Funcp) {
    ನಕ್ಷೆ_Remove_Redundant{ಇದು, ಅಂಚು_Funcp, ಸುಳ್ಳು};
}
ಶೂನ್ಯ V3_ನಕ್ಷೆ::remove_Redundant_ಅಂಚುಗಳು_ಮೊತ್ತ(V3_ಅಂಚು_Func_P ಅಂಚು_Funcp) {
    ನಕ್ಷೆ_Remove_Redundant{ಇದು, ಅಂಚು_Funcp, ನಿಜ};
}

//######################################################################
//######################################################################
// Algorithms - remove transitive

ವರ್ಗ ನಕ್ಷೆ_Alg_Remove_ಸಂಕ್ರಾಮಿಕ_ಅಂಚುಗಳು final : ನಕ್ಷೆ_Alg<> {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ ನಕ್ಷೆ_Alg_Remove_ಸಂಕ್ರಾಮಿಕ_ಅಂಚುಗಳು(V3_ನಕ್ಷೆ* graphp)
        : ನಕ್ಷೆ_Alg<>(graphp, ಶೂನ್ಯಸೂಚಕ) {}
    ಶೂನ್ಯ go() {
        ನಕ್ಷೆ_ಮಾರ್ಗ_Checker checker{m_graphp};
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_graphp->vertices()) {
            V3_ನಕ್ಷೆ_ಅಂಚು* deletep = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtx.ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                ಆದರೆ (deletep) VL_DO_CLEAR(deletep->unlinkDelete(), deletep = ಶೂನ್ಯಸೂಚಕ);
                // It should be safe to modify the graph, despite using
                // the GraphPathChecker, as none of the modifications will
                // change what can be reached from what, nor should they
                // change the rank or CP of any node.
                ಆದರೆ (checker.is_ಸಂಕ್ರಾಮಿಕ_ಅಂಚು(&ಅಂಚು)) deletep = &ಅಂಚು;
            }
            ಆದರೆ (deletep) VL_DO_DANGLING(deletep->unlinkDelete(), deletep);
        }
    }

ಖಾಸಗಿ:
    VL_UNCOPYABLE(ನಕ್ಷೆ_Alg_Remove_ಸಂಕ್ರಾಮಿಕ_ಅಂಚುಗಳು);
};

ಶೂನ್ಯ V3_ನಕ್ಷೆ::remove_ಸಂಕ್ರಾಮಿಕ_ಅಂಚುಗಳು() { ನಕ್ಷೆ_Alg_Remove_ಸಂಕ್ರಾಮಿಕ_ಅಂಚುಗಳು{ಇದು}.go(); }

//######################################################################
//######################################################################
// Algorithms - weakly connected components
// Changes color()

ವರ್ಗ ನಕ್ಷೆ_Alg_Weakly final : ನಕ್ಷೆ_Alg<> {
    // Sentinel color, meaning not colored yet. Colors themselves are 0 .. m_numColors-1.
    ಸ್ಥಿರ constexpr uint32_t UNCOLORED = std::numeric_limits<uint32_t>::max();

    uint32_t m_ಅಂಕೆ_Colors = 0;  // Number of colors assigned

    ಶೂನ್ಯ ಮುಖ್ಯ() {
        // Initialize state
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : m_graphp->vertices()) ಶೃಂಗ.color(UNCOLORED);
        // Color graph, without gaps
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : m_graphp->vertices()) {
            ಆದರೆ (ಶೃಂಗ.color() == UNCOLORED) ಶೃಂಗ_Iterate(&ಶೃಂಗ, m_ಅಂಕೆ_Colors++);
        }
    }

    ಶೂನ್ಯ ಶೃಂಗ_Iterate(V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp, uint32_t currentColor) {
        // Assign new color to each unvisited node
        // then visit each of its edges, giving them the same color
        ಆದರೆ (vertexp->color() != UNCOLORED) ಹಿಂತಿರುಗಿ;  // Already colored it
        vertexp->color(currentColor);
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            ಆದರೆ (follow_ಅಂಚು(&ಅಂಚು)) ಶೃಂಗ_Iterate(ಅಂಚು.ತುದಿ(), currentColor);
        }
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vertexp->ಒಳ_ಅಂಚುಗಳು()) {
            ಆದರೆ (follow_ಅಂಚು(&ಅಂಚು)) ಶೃಂಗ_Iterate(ಅಂಚು.fromp(), currentColor);
        }
    }

ಸಾರ್ವಜನಿಕ:
    ನಕ್ಷೆ_Alg_Weakly(V3_ನಕ್ಷೆ* graphp, V3_ಅಂಚು_Func_P ಅಂಚು_Funcp)
        : ನಕ್ಷೆ_Alg<>(graphp, ಅಂಚು_Funcp) {
        ಮುಖ್ಯ();
    }
    ~ನಕ್ಷೆ_Alg_Weakly() = ಪೂರ್ವನಿಯೋಜಿತ;

    uint32_t ಅಂಕೆ_Colors() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಅಂಕೆ_Colors; }
};

uint32_t V3_ನಕ್ಷೆ::weaklyConnected(V3_ಅಂಚು_Func_P ಅಂಚು_Funcp) {
    ಹಿಂತಿರುಗಿ ನಕ್ಷೆ_Alg_Weakly{ಇದು, ಅಂಚು_Funcp}.ಅಂಕೆ_Colors();
}

//######################################################################
//######################################################################
// Algorithms - strongly connected components
// Changes user() and color()

ವರ್ಗ ನಕ್ಷೆ_Alg_Strongly final : ನಕ್ಷೆ_Alg<> {
    uint32_t m_currentDfs = 0;  // DFS count
    std::ಸದಿಶ<V3_ನಕ್ಷೆ_ಶೃಂಗ*> m_callTrace;  // List of everything we hit processing so far

    ಶೂನ್ಯ ಮುಖ್ಯ() {
        // Use Pearce's algorithm to color the strongly connected components. For reference see
        // "An Improved Algorithm for Finding the Strongly Connected Components of a Directed
        // Graph", David J.Pearce, 2005
        //
        // Node State:
        //     Vertex::user     // DFS number indicating possible root of subtree, 0=not iterated
        //     Vertex::color    // Output subtree number (fully processed)

        // Clear info
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : m_graphp->vertices()) {
            ಶೃಂಗ.color(0);
            ಶೃಂಗ.user(0);
        }
        // Color graph
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : m_graphp->vertices()) {
            ಆದರೆ (!ಶೃಂಗ.user()) {
                m_currentDfs++;
                ಶೃಂಗ_Iterate(&ಶೃಂಗ);
            }
        }
        // If there's a single vertex of a color, it doesn't need a subgraph
        // This simplifies the consumer's code, and reduces graph debugging clutter
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : m_graphp->vertices()) {
            ತಾರ್ಕಿಕ onecolor = ನಿಜ;
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : ಶೃಂಗ.ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                ಆದರೆ (follow_ಅಂಚು(&ಅಂಚು)) {
                    ಆದರೆ (ಶೃಂಗ.color() == ಅಂಚು.ತುದಿ()->color()) {
                        onecolor = ಸುಳ್ಳು;
                        ನಿಲ್ಲಿಸು;
                    }
                }
            }
            ಆದರೆ (onecolor) ಶೃಂಗ.color(0);
        }
    }

    ಶೂನ್ಯ ಶೃಂಗ_Iterate(V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp) {
        ಸ್ಥಿರಾಂಕ uint32_t this_Dfs_ಅಂಕೆ = m_currentDfs++;
        vertexp->user(this_Dfs_ಅಂಕೆ);
        vertexp->color(0);
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            ಆದರೆ (follow_ಅಂಚು(&ಅಂಚು)) {
                V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ = ಅಂಚು.ತುದಿ();
                ಆದರೆ (!ತುದಿ->user()) {  // Dest not computed yet
                    ಶೃಂಗ_Iterate(ತುದಿ);
                }
                ಆದರೆ (!ತುದಿ->color()) {  // Dest not in a component
                    ಆದರೆ (vertexp->user() > ತುದಿ->user()) vertexp->user(ತುದಿ->user());
                }
            }
        }
        ಆದರೆ (vertexp->user() == this_Dfs_ಅಂಕೆ) {  // New head of subtree
            vertexp->color(this_Dfs_ಅಂಕೆ);  // Mark as component
            ಆಗ (!m_callTrace.ಖಾಲಿಗಳು()) {
                V3_ನಕ್ಷೆ_ಶೃಂಗ* ತೆಗೆ_Vertexp = m_callTrace.ಹಿಂಭಾಗ();
                ಆದರೆ (ತೆಗೆ_Vertexp->user() >= this_Dfs_ಅಂಕೆ) {  // Lower node is part of this subtree
                    m_callTrace.ತೆಗೆ_ಹಿಂಭಾಗ();
                    ತೆಗೆ_Vertexp->color(this_Dfs_ಅಂಕೆ);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ನಿಲ್ಲಿಸು;
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ {  // In another subtree (maybe...)
            m_callTrace.ತಳ್ಳು_ಹಿಂಭಾಗ(vertexp);
        }
    }

ಸಾರ್ವಜನಿಕ:
    ನಕ್ಷೆ_Alg_Strongly(V3_ನಕ್ಷೆ* graphp, V3_ಅಂಚು_Func_P ಅಂಚು_Funcp)
        : ನಕ್ಷೆ_Alg<>{graphp, ಅಂಚು_Funcp} {
        ಮುಖ್ಯ();
    }
    ~ನಕ್ಷೆ_Alg_Strongly() = ಪೂರ್ವನಿಯೋಜಿತ;
};

ಶೂನ್ಯ V3_ನಕ್ಷೆ::stronglyConnected(V3_ಅಂಚು_Func_P ಅಂಚು_Funcp) { ನಕ್ಷೆ_Alg_Strongly{ಇದು, ಅಂಚು_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - ranking
// Changes user() and rank()

ವರ್ಗ ನಕ್ಷೆ_Alg_Rank final : ನಕ್ಷೆ_Alg<> {
    ಶೂನ್ಯ ಮುಖ್ಯ() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : m_graphp->vertices()) {
            ಶೃಂಗ.rank(0);
            ಶೃಂಗ.user(0);
        }
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : m_graphp->vertices()) {
            ಆದರೆ (!ಶೃಂಗ.user()) ಶೃಂಗ_Iterate(&ಶೃಂಗ, 1);
        }
    }
    ಶೂನ್ಯ ಶೃಂಗ_Iterate(V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // If larger rank is found, assign it and loop back through
        // If we hit a back node make a list of all loops
        ಆದರೆ (vertexp->user() == 1) {
            m_graphp->loops_ಸಂದೇಶ_Cb(vertexp, m_ಅಂಚು_Funcp);
            ಹಿಂತಿರುಗಿ;  // LCOV_EXCL_LINE  // gcc gprof bug misses this return
        }
        ಆದರೆ (vertexp->rank() >= currentRank) ಹಿಂತಿರುಗಿ;  // Already processed it
        vertexp->user(1);
        vertexp->rank(currentRank);
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            ಆದರೆ (follow_ಅಂಚು(&ಅಂಚು)) ಶೃಂಗ_Iterate(ಅಂಚು.ತುದಿ(), currentRank + vertexp->rankAdder());
        }
        vertexp->user(2);
    }

ಸಾರ್ವಜನಿಕ:
    ನಕ್ಷೆ_Alg_Rank(V3_ನಕ್ಷೆ* graphp, V3_ಅಂಚು_Func_P ಅಂಚು_Funcp)
        : ನಕ್ಷೆ_Alg<>{graphp, ಅಂಚು_Funcp} {
        ಮುಖ್ಯ();
    }
    ~ನಕ್ಷೆ_Alg_Rank() = ಪೂರ್ವನಿಯೋಜಿತ;
};

ಶೂನ್ಯ V3_ನಕ್ಷೆ::rank() { rank(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue); }
ಶೂನ್ಯ V3_ನಕ್ಷೆ::rank(V3_ಅಂಚು_Func_P ಅಂಚು_Funcp) { ನಕ್ಷೆ_Alg_Rank{ಇದು, ಅಂಚು_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - ranking min
// Changes user() and rank()

ವರ್ಗ ನಕ್ಷೆ_Alg_Rank_Min final : ನಕ್ಷೆ_Alg<> {
    ಶೂನ್ಯ ಮುಖ್ಯ() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : m_graphp->vertices()) {
            ಶೃಂಗ.rank(0);
            ಶೃಂಗ.user(0);
        }
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : m_graphp->vertices()) {
            ಆದರೆ (!ಶೃಂಗ.user()) ಶೃಂಗ_Iterate(&ಶೃಂಗ);
        }
    }
    uint32_t ಶೃಂಗ_Iterate(V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp) {
        // Assign rank to each unvisited node
        // If we hit a back node make a list of all loops
        ಆದರೆ (vertexp->user() == 1) {
            m_graphp->loops_ಸಂದೇಶ_Cb(vertexp, m_ಅಂಚು_Funcp);
            ಹಿಂತಿರುಗಿ vertexp->rank();
        }
        ಆದರೆ (vertexp->user()) ಹಿಂತಿರುಗಿ vertexp->rank();  // Done earlier
        vertexp->user(1);
        vertexp->rank(1);  // In case loop
        // If no input edges, then rank 1 (+ adder)
        // Otherwise, get minimum from following all inputs.
        uint32_t minrank = ~0U;
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vertexp->ಒಳ_ಅಂಚುಗಳು()) {
            ಆದರೆ (follow_ಅಂಚು(&ಅಂಚು)) {
                ಸ್ಥಿರಾಂಕ uint32_t nrank = ಶೃಂಗ_Iterate(ಅಂಚು.fromp());
                minrank = std::min(minrank, nrank);
            }
        }
        ಆದರೆ (minrank == ~0U) minrank = 0;
        vertexp->rank(minrank + vertexp->rankAdder());
        vertexp->user(2);
        ಹಿಂತಿರುಗಿ vertexp->rank();
    }

ಸಾರ್ವಜನಿಕ:
    ನಕ್ಷೆ_Alg_Rank_Min(V3_ನಕ್ಷೆ* graphp, V3_ಅಂಚು_Func_P ಅಂಚು_Funcp)
        : ನಕ್ಷೆ_Alg<>{graphp, ಅಂಚು_Funcp} {
        ಮುಖ್ಯ();
    }
    ~ನಕ್ಷೆ_Alg_Rank_Min() = ಪೂರ್ವನಿಯೋಜಿತ;
};

ಶೂನ್ಯ V3_ನಕ್ಷೆ::rankMin() { rankMin(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue); }
ಶೂನ್ಯ V3_ನಕ್ಷೆ::rankMin(V3_ಅಂಚು_Func_P ಅಂಚು_Funcp) { ನಕ್ಷೆ_Alg_Rank_Min{ಇದು, ಅಂಚು_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - report loops
// Changes user()

ವರ್ಗ ನಕ್ಷೆ_Alg_RLoops final : ನಕ್ಷೆ_Alg<> {
    std::ಸದಿಶ<V3_ನಕ್ಷೆ_ಶೃಂಗ*> m_callTrace;  // List of everything we hit processing so far
    std::ಸದಿಶ<string> m_msgs;  // Output messages
    ತಾರ್ಕಿಕ m_ಮುಗಿದ = ಸುಳ್ಳು;  // Exit algorithm

    ಶೂನ್ಯ ಮುಖ್ಯ(V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp) {
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        m_graphp->userClearVertices();
        m_callTrace.reserve(100);
        ಶೃಂಗ_Iterate(vertexp, 0);
    }

    ಶೂನ್ಯ ಶೃಂಗ_Iterate(V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // When we hit ourself again, return the list of all loops
        ಆದರೆ (m_ಮುಗಿದ) ಹಿಂತಿರುಗಿ;

        // Can't just reserve(), unless we modify size() before setting array directly
        ಆಗ (m_callTrace.ಗಾತ್ರ_2() <= currentRank) m_callTrace.ತಳ್ಳು_ಹಿಂಭಾಗ(vertexp);
        m_callTrace[currentRank++] = vertexp;

        ಆದರೆ (vertexp->user() == 1) {
            ಗಾಗಿ (ಚಿಹ್ನೆರಹಿತ i = 0; i < currentRank; i++)
                m_msgs.emplace_ಹಿಂಭಾಗ(m_graphp->loops_ಶೃಂಗ_Cb(m_callTrace[i]));
            m_ಮುಗಿದ = ನಿಜ;
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (vertexp->user() == 2) ಹಿಂತಿರುಗಿ;  // Already processed it
        vertexp->user(1);
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            ಆದರೆ (follow_ಅಂಚು(&ಅಂಚು)) ಶೃಂಗ_Iterate(ಅಂಚು.ತುದಿ(), currentRank);
        }
        vertexp->user(2);
    }

ಸಾರ್ವಜನಿಕ:
    ನಕ್ಷೆ_Alg_RLoops(V3_ನಕ್ಷೆ* graphp, V3_ಅಂಚು_Func_P ಅಂಚು_Funcp, V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp)
        : ನಕ್ಷೆ_Alg<>{graphp, ಅಂಚು_Funcp} {
        ಮುಖ್ಯ(vertexp);
    }
    ~ನಕ್ಷೆ_Alg_RLoops() = ಪೂರ್ವನಿಯೋಜಿತ;
    string ಸಂದೇಶ() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ std::accumulate(m_msgs.begin(), m_msgs.ಕೊನೆ(), std::string{""});
    }
};

string V3_ನಕ್ಷೆ::reportLoops(V3_ಅಂಚು_Func_P ಅಂಚು_Funcp, V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp) {
    ಹಿಂತಿರುಗಿ ನಕ್ಷೆ_Alg_RLoops{ಇದು, ಅಂಚು_Funcp, vertexp}.ಸಂದೇಶ();
}

//######################################################################
//######################################################################
// Algorithms - subtrees
// Changes user()

ವರ್ಗ ನಕ್ಷೆ_Alg_Subtrees final : ನಕ್ಷೆ_Alg<> {
    V3_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ m_loopGraphp;

    //! Iterate through all connected nodes of a graph with a loop or loops.
    V3_ನಕ್ಷೆ_ಶೃಂಗ* ಶೃಂಗ_Iterate_All(V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp) {
        ಆದರೆ (V3_ನಕ್ಷೆ_ಶೃಂಗ* newVertexp = static_cast<V3_ನಕ್ಷೆ_ಶೃಂಗ*>(vertexp->userp())) {
            ಹಿಂತಿರುಗಿ newVertexp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            newVertexp = vertexp->clone(m_loopGraphp);
            vertexp->userp(newVertexp);

            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                ಆದರೆ (follow_ಅಂಚು(&ಅಂಚು)) {
                    V3_ನಕ್ಷೆ_ಅಂಚು* newEdgep = static_cast<V3_ನಕ್ಷೆ_ಅಂಚು*>(ಅಂಚು.userp());
                    ಆದರೆ (!newEdgep) {
                        V3_ನಕ್ಷೆ_ಶೃಂಗ* new_ತುದಿ = ಶೃಂಗ_Iterate_All(ಅಂಚು.ತುದಿ());
                        newEdgep = ಅಂಚು.clone(m_loopGraphp, newVertexp, new_ತುದಿ);
                        ಅಂಚು.userp(newEdgep);
                    }
                }
            }
            ಹಿಂತಿರುಗಿ newVertexp;
        }
    }

ಸಾರ್ವಜನಿಕ:
    ನಕ್ಷೆ_Alg_Subtrees(V3_ನಕ್ಷೆ* graphp, V3_ನಕ್ಷೆ* loopGraphp, V3_ಅಂಚು_Func_P ಅಂಚು_Funcp,
                     V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp)
        : ನಕ್ಷೆ_Alg<>{graphp, ಅಂಚು_Funcp}
        , m_loopGraphp{loopGraphp} {
        // Vertex::m_userp - New vertex if we have seen this vertex already
        // Edge::m_userp - New edge if we have seen this edge already
        m_graphp->userClearVertices();
        m_graphp->user_Clear_ಅಂಚುಗಳು();
        (ಶೂನ್ಯ)ಶೃಂಗ_Iterate_All(vertexp);
    }
    ~ನಕ್ಷೆ_Alg_Subtrees() = ಪೂರ್ವನಿಯೋಜಿತ;
};

//! Report the entire connected graph with a loop or loops
ಶೂನ್ಯ V3_ನಕ್ಷೆ::subtreeLoops(V3_ಅಂಚು_Func_P ಅಂಚು_Funcp, V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp, V3_ನಕ್ಷೆ* loopGraphp) {
    ನಕ್ಷೆ_Alg_Subtrees{ಇದು, loopGraphp, ಅಂಚು_Funcp, vertexp};
}

//######################################################################
//######################################################################
// Algorithms - sorting

ಶೂನ್ಯ V3_ನಕ್ಷೆ::ವಿಂಗಡಿಸು_Vertices() {
    // Sort list of vertices by rank, then fanout
    std::ಸದಿಶ<V3_ನಕ್ಷೆ_ಶೃಂಗ*> vertexps;
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : m_vertices) vertexps.ತಳ್ಳು_ಹಿಂಭಾಗ(&ಶೃಂಗ);
    std::stable_ವಿಂಗಡಿಸು(vertexps.begin(), vertexps.ಕೊನೆ(),
                     [](ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* rhsp) {  //
                         ಹಿಂತಿರುಗಿ lhsp->ವಿಂಗಡಿಸು_Cmp(rhsp) < 0;
                     });
    // Re-insert in sorted order
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vertexp : vertexps) {
        m_vertices.unlink(vertexp);
        m_vertices.link_ಹಿಂಭಾಗ(vertexp);
    }
}

ಶೂನ್ಯ V3_ನಕ್ಷೆ::ವಿಂಗಡಿಸು_ಅಂಚುಗಳು() {
    // Sort edges by rank then fanout of node they point to
    std::ಸದಿಶ<V3_ನಕ್ಷೆ_ಅಂಚು*> ಅಂಚುಗಳು;
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : vertices()) {
        // Make a vector
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : ಶೃಂಗ.ಹೊರಹೋಗು_ಅಂಚುಗಳು()) ಅಂಚುಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(&ಅಂಚು);
        //ವಿಂಗಡಿಸು
        std::stable_ವಿಂಗಡಿಸು(ಅಂಚುಗಳು.begin(), ಅಂಚುಗಳು.ಕೊನೆ(),
                         [](ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು* lhsp, ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು* rhsp) {  //
                             ಹಿಂತಿರುಗಿ lhsp->ವಿಂಗಡಿಸು_Cmp(rhsp) < 0;
                         });
        // Relink edges in specified order
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep : ಅಂಚುಗಳು) edgep->relinkFromp(&ಶೃಂಗ);
        // Prep for next
        ಅಂಚುಗಳು.clear();
    }
}

//######################################################################
//######################################################################
// Algorithms - ordering
//      Compute near optimal ordering of the nodes, where:
//          If a required    edge is A->B, rank(A)<rank(B)
//          Visit edges and assign ranks to keep minimal crossings
//              (Results in better dcache packing.)

ಶೂನ್ಯ V3_ನಕ್ಷೆ::ಕ್ರಮ() {
    UINFO(2, "Order:");

    // Compute rankings again
    rank(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);
    ಕ್ರಮ_Pre_Ranked();
}

ಶೂನ್ಯ V3_ನಕ್ಷೆ::ಕ್ರಮ_Pre_Ranked() {
    // Compute fanouts
    // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
    userClearVertices();
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : vertices()) {
        ಆದರೆ (!ಶೃಂಗ.user()) ಕ್ರಮ_DFSIterate(&ಶೃಂಗ);
    }

    // Sort list of vertices by rank, then fanout. Fanout is a bit of a
    // misnomer. It is the sum of all the fanouts of nodes reached from a node
    // *plus* the count of edges in to that node.
    ವಿಂಗಡಿಸು_Vertices();
    // Sort edges by rank then fanout of node they point to
    ವಿಂಗಡಿಸು_ಅಂಚುಗಳು();
}

ದ್ವಿಗುಣ V3_ನಕ್ಷೆ::ಕ್ರಮ_DFSIterate(V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp) {
    // Compute fanouts of each node
    // If forward edge, don't double count that fanout
    ಆದರೆ (vertexp->user() == 2) ಹಿಂತಿರುಗಿ vertexp->fanout();  // Already processed it
    UASSERT_OBJ(vertexp->user() != 1, vertexp, "Loop found, backward edges should be dead");
    vertexp->user(1);
    ದ್ವಿಗುಣ fanout = 0;
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
        ಆದರೆ (ಅಂಚು.ತೂಕ_2()) fanout += ಕ್ರಮ_DFSIterate(ಅಂಚು.ತುದಿ());
    }
    // Just count inbound edges
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vertexp->ಒಳ_ಅಂಚುಗಳು()) {
        ಆದರೆ (ಅಂಚು.ತೂಕ_2()) ++fanout;
    }
    vertexp->fanout(fanout);
    vertexp->user(2);
    ಹಿಂತಿರುಗಿ vertexp->fanout();
}

//######################################################################
//######################################################################
// Algorithms - parallelism report

ವರ್ಗ ನಕ್ಷೆ_Alg_Parallelism_Report final {
    // MEMBERS
    V3_ನಕ್ಷೆ& m_ನಕ್ಷೆ;  // The graph
    ಸ್ಥಿರಾಂಕ std::function<uint64_t(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ*)> m_ಶೃಂಗ_ವೆಚ್ಚ;  // vertex cost function
    V3_ನಕ್ಷೆ::ParallelismReport m_report;  // The result report

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ನಕ್ಷೆ_Alg_Parallelism_Report(V3_ನಕ್ಷೆ& ನಕ್ಷೆ,
                                       std::function<uint64_t(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ*)> ಶೃಂಗ_ವೆಚ್ಚ)
        : m_ನಕ್ಷೆ{ನಕ್ಷೆ}
        , m_ಶೃಂಗ_ವೆಚ್ಚ{ಶೃಂಗ_ವೆಚ್ಚ} {
        // For each node, record the critical path cost from the start
        // of the graph through the end of the node.
        std::unordered_map<ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ*, uint64_t> critPaths;
        ನಕ್ಷೆ_ಹರಿವು_Unordered serialize{&m_ನಕ್ಷೆ};
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp; (vertexp = serialize.nextp());) {
            ++m_report.m_ಶೃಂಗ_ಎಣಿಕೆ;
            uint64_t cp_ವೆಚ್ಚ_ಗಮ್ಯ_ಇಲ್ಲಿ = 0;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vertexp->ಒಳ_ಅಂಚುಗಳು()) {
                ++m_report.m_ಅಂಚು_ಎಣಿಕೆ;
                // For each upstream item, add its critical path cost to
                // the cost of this edge, to form a new candidate critical
                // path cost to the current node. Whichever is largest is
                // the critical path to reach the start of this node.
                cp_ವೆಚ್ಚ_ಗಮ್ಯ_ಇಲ್ಲಿ = std::max(cp_ವೆಚ್ಚ_ಗಮ್ಯ_ಇಲ್ಲಿ, critPaths[ಅಂಚು.fromp()]);
            }
            // Include the cost of the current vertex in the critical
            // path, so it represents the critical path to the end of
            // this vertex.
            cp_ವೆಚ್ಚ_ಗಮ್ಯ_ಇಲ್ಲಿ += m_ಶೃಂಗ_ವೆಚ್ಚ(vertexp);
            critPaths[vertexp] = cp_ವೆಚ್ಚ_ಗಮ್ಯ_ಇಲ್ಲಿ;
            m_report.m_critical_ಮಾರ್ಗ_ವೆಚ್ಚ = std::max(m_report.m_critical_ಮಾರ್ಗ_ವೆಚ್ಚ, cp_ವೆಚ್ಚ_ಗಮ್ಯ_ಇಲ್ಲಿ);
            // Tally the total cost contributed by vertices.
            m_report.m_ಒಟ್ಟು_ನಕ್ಷೆ_ವೆಚ್ಚ += m_ಶೃಂಗ_ವೆಚ್ಚ(vertexp);
        }
    }
    ~ನಕ್ಷೆ_Alg_Parallelism_Report() = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(ನಕ್ಷೆ_Alg_Parallelism_Report);
    VL_UNMOVABLE(ನಕ್ಷೆ_Alg_Parallelism_Report);

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ V3_ನಕ್ಷೆ::ParallelismReport
    apply(V3_ನಕ್ಷೆ& ನಕ್ಷೆ, std::function<uint32_t(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ*)> ಶೃಂಗ_ವೆಚ್ಚ) {
        ಹಿಂತಿರುಗಿ ನಕ್ಷೆ_Alg_Parallelism_Report(ನಕ್ಷೆ, ಶೃಂಗ_ವೆಚ್ಚ).m_report;
    }
};

V3_ನಕ್ಷೆ::ParallelismReport
V3_ನಕ್ಷೆ::parallelismReport(std::function<uint64_t(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ*)> ಶೃಂಗ_ವೆಚ್ಚ) {
    ಹಿಂತಿರುಗಿ ನಕ್ಷೆ_Alg_Parallelism_Report::apply(*ಇದು, ಶೃಂಗ_ವೆಚ್ಚ);
}
