// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Cycle finding algorithm for DfgGraph
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
//
// Implements Pearce's algorithm to color the strongly connected components. For reference
// see "An Improved Algorithm for Finding the Strongly Connected Components of a Directed
// Graph", David J.Pearce, 2005.
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3Dfg.h"
#ಅನ್ತರ್ಭಾವ "V3DfgPasses.h"

#ಅನ್ತರ್ಭಾವ <limits>
#ಅನ್ತರ್ಭಾವ <vector>

ವರ್ಗ ColorStronglyConnectedComponents final {
    static_assert(ಗಾತ್ರ(uint32_t[2]) == ಗಾತ್ರ(uint64_t), "Incorrect overlay size");

    // CONSTANTS
    ಸ್ಥಿರ constexpr uint32_t UNASSIGNED = std::numeric_limits<uint32_t>::max();

    // STATE
    ಸ್ಥಿರಾಂಕ Dfg_ನಕ್ಷೆ& m_dfg;  // The input graph
    DfgUserMap<uint64_t>& m_map;  // The result map we are computing - also used for traversal
    uint32_t m_nonTrivialSCCs = 0;  // Number of non-trivial SCCs in the graph
    uint32_t m_ಸೂಚ್ಯಂಕ = 0;  // Visitation index counter
    std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*> m_ರಾಶಿ;  // The stack used by the algorithm

    // METHODS
    // Use the bottom 32-bit word as the component number
    uint32_t& component(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) {  //
        ಹಿಂತಿರುಗಿ reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[0];
    }
    // Use the top 32-bit word as the visitation index
    uint32_t& ಸೂಚ್ಯಂಕ(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) {  //
        ಹಿಂತಿರುಗಿ reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[1];
    }

    ಶೂನ್ಯ ಭೇಟಿ_Color_SCCs(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) {
        UDEBUGONLY(UASSERT_OBJ(ಸೂಚ್ಯಂಕ(vtx) == UNASSIGNED, &vtx, "Already visited vertex"););

        // Visiting vertex
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಬೇರು_ಸೂಚ್ಯಂಕ = ಸೂಚ್ಯಂಕ(vtx) = ++m_ಸೂಚ್ಯಂಕ;

        // Visit children
        vtx.foreachSink([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& ಮಗು) {
            // If the child has not yet been visited, then continue traversal
            ಆದರೆ (ಸೂಚ್ಯಂಕ(ಮಗು) == UNASSIGNED) ಭೇಟಿ_Color_SCCs(ಮಗು);
            // If the child is not in an SCC
            ಆದರೆ (component(ಮಗು) == UNASSIGNED) {
                ಆದರೆ (ಸೂಚ್ಯಂಕ(vtx) > ಸೂಚ್ಯಂಕ(ಮಗು)) ಸೂಚ್ಯಂಕ(vtx) = ಸೂಚ್ಯಂಕ(ಮಗು);
            }
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });

        ಆದರೆ (ಸೂಚ್ಯಂಕ(vtx) == ಬೇರು_ಸೂಚ್ಯಂಕ) {
            // This is the 'root' of an SCC

            // A trivial SCC contains only a single vertex
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isTrivial = m_ರಾಶಿ.ಖಾಲಿಗಳು() || ಸೂಚ್ಯಂಕ(*m_ರಾಶಿ.ಹಿಂಭಾಗ()) < ಬೇರು_ಸೂಚ್ಯಂಕ;
            // We also need a separate component for vertices that drive themselves (which can
            // happen for input like 'assign a = a'), as we want to extract them (they are cyclic).
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ drivesSelf = vtx.foreachSink([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& sink) {  //
                ಹಿಂತಿರುಗಿ &vtx == &sink;
            });

            ಆದರೆ (!isTrivial || drivesSelf) {
                // Allocate new component
                ++m_nonTrivialSCCs;
                component(vtx) = m_nonTrivialSCCs;
                ಆಗ (!m_ರಾಶಿ.ಖಾಲಿಗಳು()) {
                    // Only higher nodes belong to the same SCC
                    ಆದರೆ (ಸೂಚ್ಯಂಕ(*m_ರಾಶಿ.ಹಿಂಭಾಗ()) < ಬೇರು_ಸೂಚ್ಯಂಕ) ನಿಲ್ಲಿಸು;
                    component(*m_ರಾಶಿ.ಹಿಂಭಾಗ()) = m_nonTrivialSCCs;
                    m_ರಾಶಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Trivial SCC (and does not drive itself), so acyclic. Keep it in original graph.
                component(vtx) = 0;
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Not the root of an SCC
            m_ರಾಶಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(&vtx);
        }
    }

    ಶೂನ್ಯ colorSCCs() {
        // We know constant nodes have no input edges, so they cannot be part
        // of a non-trivial SCC. Mark them as such without any real traversals.
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ DfgConst& vtx : m_dfg.constVertices()) {
            ಸೂಚ್ಯಂಕ(vtx) = 0;
            component(vtx) = 0;
        }

        // We know ast references have no output or input edges
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Ast& vtx : m_dfg.astVertices()) {
            ಸೂಚ್ಯಂಕ(vtx) = 0;
            component(vtx) = 0;
        }

        // Initialize state of variable vertices
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var& vtx : m_dfg.varVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            ಆದರೆ ((!vtx.srcp() && !vtx.defaultp()) || !vtx.hasSinks()) {
                ಸೂಚ್ಯಂಕ(vtx) = 0;
                component(vtx) = 0;
                ಮುಂದುವರಿ;
            }
            ಸೂಚ್ಯಂಕ(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Initialize state of operation vertices
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx : m_dfg.opVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            ಆದರೆ (!vtx.nInputs() || !vtx.hasSinks()) {
                ಸೂಚ್ಯಂಕ(vtx) = 0;
                component(vtx) = 0;
                ಮುಂದುವರಿ;
            }
            ಸೂಚ್ಯಂಕ(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Start traversals through not yet visited variables
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var& vtx : m_dfg.varVertices()) {
            ಆದರೆ (ಸೂಚ್ಯಂಕ(vtx) == UNASSIGNED) ಭೇಟಿ_Color_SCCs(vtx);
        }

        // Start traversals through not yet visited operations
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx : m_dfg.opVertices()) {
            ಆದರೆ (ಸೂಚ್ಯಂಕ(vtx) == UNASSIGNED) ಭೇಟಿ_Color_SCCs(vtx);
        }
    }

    ಸ್ಪಷ್ಟ ColorStronglyConnectedComponents(ಸ್ಥಿರಾಂಕ Dfg_ನಕ್ಷೆ& dfg, DfgUserMap<uint64_t>& map)
        : m_dfg{dfg}
        , m_map{map} {
        UASSERT(dfg.ಗಾತ್ರ_2() < UNASSIGNED, "Graph too big " << dfg.name());
        // Yet another implementation of Pearce's algorithm.
        colorSCCs();
        // Re-assign mapped values
        m_dfg.for_Each_ಶೃಂಗ([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) {
            ಸ್ಥಿರಾಂಕ uint64_t c = component(vtx);
            map[vtx] = c;
        });
    }

ಸಾರ್ವಜನಿಕ:
    // See declaration of  V3DfgPasses::colorStronglyConnectedComponents
    ಸ್ಥಿರ uint32_t apply(ಸ್ಥಿರಾಂಕ Dfg_ನಕ್ಷೆ& dfg, DfgUserMap<uint64_t>& map) {
        ಹಿಂತಿರುಗಿ ColorStronglyConnectedComponents{dfg, map}.m_nonTrivialSCCs;
    }
};

uint32_t V3DfgPasses::colorStronglyConnectedComponents(ಸ್ಥಿರಾಂಕ Dfg_ನಕ್ಷೆ& dfg,
                                                       DfgUserMap<uint64_t>& map) {
    ಹಿಂತಿರುಗಿ ColorStronglyConnectedComponents::apply(dfg, map);
}
