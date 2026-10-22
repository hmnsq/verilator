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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dfg.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgPasses.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <limits>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

ਵਰਗ ColorStronglyConnectedComponents final {
    static_assert(ਆਕਾਰ(uint32_t[2]) == ਆਕਾਰ(uint64_t), "Incorrect overlay size");

    // CONSTANTS
    ਸਥਿਰ constexpr uint32_t UNASSIGNED = std::numeric_limits<uint32_t>::max();

    // STATE
    ਅਚਲ Dfg_ਗ੍ਰਾਫ& m_dfg;  // The input graph
    DfgUserMap<uint64_t>& m_map;  // The result map we are computing - also used for traversal
    uint32_t m_nonTrivialSCCs = 0;  // Number of non-trivial SCCs in the graph
    uint32_t m_ਸੂਚਕਾਂਕ = 0;  // Visitation index counter
    std::ਸਦਿਸ਼<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*> m_ਢੇਰ;  // The stack used by the algorithm

    // METHODS
    // Use the bottom 32-bit word as the component number
    uint32_t& component(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) {  //
        ਵਾਪਸ reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[0];
    }
    // Use the top 32-bit word as the visitation index
    uint32_t& ਸੂਚਕਾਂਕ(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) {  //
        ਵਾਪਸ reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[1];
    }

    ਖਾਲੀ ਦੇਖੋ_Color_SCCs(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) {
        UDEBUGONLY(UASSERT_OBJ(ਸੂਚਕਾਂਕ(vtx) == UNASSIGNED, &vtx, "Already visited vertex"););

        // Visiting vertex
        ਅਚਲ ਮਾਪ_t ਜੜ੍ਹ_ਸੂਚਕਾਂਕ = ਸੂਚਕਾਂਕ(vtx) = ++m_ਸੂਚਕਾਂਕ;

        // Visit children
        vtx.foreachSink([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& ਬੱਚਾ) {
            // If the child has not yet been visited, then continue traversal
            ਜੇ (ਸੂਚਕਾਂਕ(ਬੱਚਾ) == UNASSIGNED) ਦੇਖੋ_Color_SCCs(ਬੱਚਾ);
            // If the child is not in an SCC
            ਜੇ (component(ਬੱਚਾ) == UNASSIGNED) {
                ਜੇ (ਸੂਚਕਾਂਕ(vtx) > ਸੂਚਕਾਂਕ(ਬੱਚਾ)) ਸੂਚਕਾਂਕ(vtx) = ਸੂਚਕਾਂਕ(ਬੱਚਾ);
            }
            ਵਾਪਸ ਝੂਠ;
        });

        ਜੇ (ਸੂਚਕਾਂਕ(vtx) == ਜੜ੍ਹ_ਸੂਚਕਾਂਕ) {
            // This is the 'root' of an SCC

            // A trivial SCC contains only a single vertex
            ਅਚਲ ਤਾਰਕਿਕ isTrivial = m_ਢੇਰ.ਖਾਲੀ_ਥਾਵਾਂ() || ਸੂਚਕਾਂਕ(*m_ਢੇਰ.ਪਿੱਠ()) < ਜੜ੍ਹ_ਸੂਚਕਾਂਕ;
            // We also need a separate component for vertices that drive themselves (which can
            // happen for input like 'assign a = a'), as we want to extract them (they are cyclic).
            ਅਚਲ ਤਾਰਕਿਕ drivesSelf = vtx.foreachSink([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& sink) {  //
                ਵਾਪਸ &vtx == &sink;
            });

            ਜੇ (!isTrivial || drivesSelf) {
                // Allocate new component
                ++m_nonTrivialSCCs;
                component(vtx) = m_nonTrivialSCCs;
                ਜਦੋਂ (!m_ਢੇਰ.ਖਾਲੀ_ਥਾਵਾਂ()) {
                    // Only higher nodes belong to the same SCC
                    ਜੇ (ਸੂਚਕਾਂਕ(*m_ਢੇਰ.ਪਿੱਠ()) < ਜੜ੍ਹ_ਸੂਚਕਾਂਕ) ਰੋਕੋ;
                    component(*m_ਢੇਰ.ਪਿੱਠ()) = m_nonTrivialSCCs;
                    m_ਢੇਰ.ਕੱਢੋ_ਪਿੱਠ();
                }
            } ਨਹੀਂਤਾਂ {
                // Trivial SCC (and does not drive itself), so acyclic. Keep it in original graph.
                component(vtx) = 0;
            }
        } ਨਹੀਂਤਾਂ {
            // Not the root of an SCC
            m_ਢੇਰ.ਧੱਕੋ_ਪਿੱਠ(&vtx);
        }
    }

    ਖਾਲੀ colorSCCs() {
        // We know constant nodes have no input edges, so they cannot be part
        // of a non-trivial SCC. Mark them as such without any real traversals.
        ਲਈ (ਅਚਲ DfgConst& vtx : m_dfg.constVertices()) {
            ਸੂਚਕਾਂਕ(vtx) = 0;
            component(vtx) = 0;
        }

        // We know ast references have no output or input edges
        ਲਈ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Ast& vtx : m_dfg.astVertices()) {
            ਸੂਚਕਾਂਕ(vtx) = 0;
            component(vtx) = 0;
        }

        // Initialize state of variable vertices
        ਲਈ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var& vtx : m_dfg.varVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            ਜੇ ((!vtx.srcp() && !vtx.defaultp()) || !vtx.hasSinks()) {
                ਸੂਚਕਾਂਕ(vtx) = 0;
                component(vtx) = 0;
                ਜਾਰੀ;
            }
            ਸੂਚਕਾਂਕ(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Initialize state of operation vertices
        ਲਈ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx : m_dfg.opVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            ਜੇ (!vtx.nInputs() || !vtx.hasSinks()) {
                ਸੂਚਕਾਂਕ(vtx) = 0;
                component(vtx) = 0;
                ਜਾਰੀ;
            }
            ਸੂਚਕਾਂਕ(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Start traversals through not yet visited variables
        ਲਈ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var& vtx : m_dfg.varVertices()) {
            ਜੇ (ਸੂਚਕਾਂਕ(vtx) == UNASSIGNED) ਦੇਖੋ_Color_SCCs(vtx);
        }

        // Start traversals through not yet visited operations
        ਲਈ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx : m_dfg.opVertices()) {
            ਜੇ (ਸੂਚਕਾਂਕ(vtx) == UNASSIGNED) ਦੇਖੋ_Color_SCCs(vtx);
        }
    }

    ਸਪਸ਼ਟ ColorStronglyConnectedComponents(ਅਚਲ Dfg_ਗ੍ਰਾਫ& dfg, DfgUserMap<uint64_t>& map)
        : m_dfg{dfg}
        , m_map{map} {
        UASSERT(dfg.ਮਾਪ() < UNASSIGNED, "Graph too big " << dfg.name());
        // Yet another implementation of Pearce's algorithm.
        colorSCCs();
        // Re-assign mapped values
        m_dfg.for_Each_ਸ਼ੀਰਸ਼([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) {
            ਅਚਲ uint64_t c = component(vtx);
            map[vtx] = c;
        });
    }

ਜਨਤਕ:
    // See declaration of  V3DfgPasses::colorStronglyConnectedComponents
    ਸਥਿਰ uint32_t apply(ਅਚਲ Dfg_ਗ੍ਰਾਫ& dfg, DfgUserMap<uint64_t>& map) {
        ਵਾਪਸ ColorStronglyConnectedComponents{dfg, map}.m_nonTrivialSCCs;
    }
};

uint32_t V3DfgPasses::colorStronglyConnectedComponents(ਅਚਲ Dfg_ਗ੍ਰਾਫ& dfg,
                                                       DfgUserMap<uint64_t>& map) {
    ਵਾਪਸ ColorStronglyConnectedComponents::apply(dfg, map);
}
