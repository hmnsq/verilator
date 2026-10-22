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
    static_assert(ಪರಿಮಾಣ(uint32_t[2]) == ಪರಿಮಾಣ(uint64_t), "Incorrect overlay size");

    // CONSTANTS
    ಸ್ಥಿರ constexpr uint32_t UNASSIGNED = std::numeric_limits<uint32_t>::max();

    // STATE
    ಧ್ರುವ DfgGraph& m_dfg;  // The input graph
    DfgUserMap<uint64_t>& m_map;  // The result map we are computing - also used for traversal
    uint32_t m_nonTrivialSCCs = 0;  // Number of non-trivial SCCs in the graph
    uint32_t m_index = 0;  // Visitation index counter
    std::vector<ಧ್ರುವ DfgVertex*> m_ರಾಶಿ;  // The stack used by the algorithm

    // METHODS
    // Use the bottom 32-bit word as the component number
    uint32_t& component(ಧ್ರುವ DfgVertex& vtx) {  //
        ಪ್ರತ್ಯಾಗಮ reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[0];
    }
    // Use the top 32-bit word as the visitation index
    uint32_t& index(ಧ್ರುವ DfgVertex& vtx) {  //
        ಪ್ರತ್ಯಾಗಮ reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[1];
    }

    ಶೂನ್ಯ visitColorSCCs(ಧ್ರುವ DfgVertex& vtx) {
        UDEBUGONLY(UASSERT_OBJ(index(vtx) == UNASSIGNED, &vtx, "Already visited vertex"););

        // Visiting vertex
        ಧ್ರುವ size_t ಬೇರು_Index = index(vtx) = ++m_index;

        // Visit children
        vtx.foreachSink([&](ಧ್ರುವ DfgVertex& child) {
            // If the child has not yet been visited, then continue traversal
            ಯದಿ (index(child) == UNASSIGNED) visitColorSCCs(child);
            // If the child is not in an SCC
            ಯದಿ (component(child) == UNASSIGNED) {
                ಯದಿ (index(vtx) > index(child)) index(vtx) = index(child);
            }
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });

        ಯದಿ (index(vtx) == ಬೇರು_Index) {
            // This is the 'root' of an SCC

            // A trivial SCC contains only a single vertex
            ಧ್ರುವ ತಾರ್ಕಿಕ isTrivial = m_ರಾಶಿ.empty() || index(*m_ರಾಶಿ.back()) < ಬೇರು_Index;
            // We also need a separate component for vertices that drive themselves (which can
            // happen for input like 'assign a = a'), as we want to extract them (they are cyclic).
            ಧ್ರುವ ತಾರ್ಕಿಕ drivesSelf = vtx.foreachSink([&](ಧ್ರುವ DfgVertex& sink) {  //
                ಪ್ರತ್ಯಾಗಮ &vtx == &sink;
            });

            ಯದಿ (!isTrivial || drivesSelf) {
                // Allocate new component
                ++m_nonTrivialSCCs;
                component(vtx) = m_nonTrivialSCCs;
                ಯಾವತ್ (!m_ರಾಶಿ.empty()) {
                    // Only higher nodes belong to the same SCC
                    ಯದಿ (index(*m_ರಾಶಿ.back()) < ಬೇರು_Index) ಭಙ್ಗ;
                    component(*m_ರಾಶಿ.back()) = m_nonTrivialSCCs;
                    m_ರಾಶಿ.ತೆಗೆ_back();
                }
            } ಅನ್ಯಥಾ {
                // Trivial SCC (and does not drive itself), so acyclic. Keep it in original graph.
                component(vtx) = 0;
            }
        } ಅನ್ಯಥಾ {
            // Not the root of an SCC
            m_ರಾಶಿ.ತಳ್ಳು_back(&vtx);
        }
    }

    ಶೂನ್ಯ colorSCCs() {
        // We know constant nodes have no input edges, so they cannot be part
        // of a non-trivial SCC. Mark them as such without any real traversals.
        ಪ್ರತಿ (ಧ್ರುವ DfgConst& vtx : m_dfg.constVertices()) {
            index(vtx) = 0;
            component(vtx) = 0;
        }

        // We know ast references have no output or input edges
        ಪ್ರತಿ (ಧ್ರುವ DfgVertexAst& vtx : m_dfg.astVertices()) {
            index(vtx) = 0;
            component(vtx) = 0;
        }

        // Initialize state of variable vertices
        ಪ್ರತಿ (ಧ್ರುವ DfgVertexVar& vtx : m_dfg.varVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            ಯದಿ ((!vtx.srcp() && !vtx.defaultp()) || !vtx.hasSinks()) {
                index(vtx) = 0;
                component(vtx) = 0;
                ಅನುವರ್ತ;
            }
            index(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Initialize state of operation vertices
        ಪ್ರತಿ (ಧ್ರುವ DfgVertex& vtx : m_dfg.opVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            ಯದಿ (!vtx.nInputs() || !vtx.hasSinks()) {
                index(vtx) = 0;
                component(vtx) = 0;
                ಅನುವರ್ತ;
            }
            index(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Start traversals through not yet visited variables
        ಪ್ರತಿ (ಧ್ರುವ DfgVertexVar& vtx : m_dfg.varVertices()) {
            ಯದಿ (index(vtx) == UNASSIGNED) visitColorSCCs(vtx);
        }

        // Start traversals through not yet visited operations
        ಪ್ರತಿ (ಧ್ರುವ DfgVertex& vtx : m_dfg.opVertices()) {
            ಯದಿ (index(vtx) == UNASSIGNED) visitColorSCCs(vtx);
        }
    }

    explicit ColorStronglyConnectedComponents(ಧ್ರುವ DfgGraph& dfg, DfgUserMap<uint64_t>& map)
        : m_dfg{dfg}
        , m_map{map} {
        UASSERT(dfg.size() < UNASSIGNED, "Graph too big " << dfg.name());
        // Yet another implementation of Pearce's algorithm.
        colorSCCs();
        // Re-assign mapped values
        m_dfg.forEachVertex([&](ಧ್ರುವ DfgVertex& vtx) {
            ಧ್ರುವ uint64_t c = component(vtx);
            map[vtx] = c;
        });
    }

ಸಾರ್ವಜನಿಕ:
    // See declaration of  V3DfgPasses::colorStronglyConnectedComponents
    ಸ್ಥಿರ uint32_t apply(ಧ್ರುವ DfgGraph& dfg, DfgUserMap<uint64_t>& map) {
        ಪ್ರತ್ಯಾಗಮ ColorStronglyConnectedComponents{dfg, map}.m_nonTrivialSCCs;
    }
};

uint32_t V3DfgPasses::colorStronglyConnectedComponents(ಧ್ರುವ DfgGraph& dfg,
                                                       DfgUserMap<uint64_t>& map) {
    ಪ್ರತ್ಯಾಗಮ ColorStronglyConnectedComponents::apply(dfg, map);
}
