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

#અન્તર્ભાવ "V3Dfg.h"
#અન્તર્ભાવ "V3DfgPasses.h"

#અન્તર્ભાવ <limits>
#અન્તર્ભાવ <vector>

વર્ગ ColorStronglyConnectedComponents final {
    static_assert(કદ(uint32_t[2]) == કદ(uint64_t), "Incorrect overlay size");

    // CONSTANTS
    સ્થિર constexpr uint32_t UNASSIGNED = std::numeric_limits<uint32_t>::max();

    // STATE
    અચલ DfgGraph& m_dfg;  // The input graph
    DfgUserMap<uint64_t>& m_map;  // The result map we are computing - also used for traversal
    uint32_t m_nonTrivialSCCs = 0;  // Number of non-trivial SCCs in the graph
    uint32_t m_index = 0;  // Visitation index counter
    std::vector<અચલ DfgVertex*> m_ઢગલો;  // The stack used by the algorithm

    // METHODS
    // Use the bottom 32-bit word as the component number
    uint32_t& component(અચલ DfgVertex& vtx) {  //
        પાછા reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[0];
    }
    // Use the top 32-bit word as the visitation index
    uint32_t& index(અચલ DfgVertex& vtx) {  //
        પાછા reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[1];
    }

    શૂન્ય visitColorSCCs(અચલ DfgVertex& vtx) {
        UDEBUGONLY(UASSERT_OBJ(index(vtx) == UNASSIGNED, &vtx, "Already visited vertex"););

        // Visiting vertex
        અચલ size_t મૂળ_Index = index(vtx) = ++m_index;

        // Visit children
        vtx.foreachSink([&](અચલ DfgVertex& child) {
            // If the child has not yet been visited, then continue traversal
            જો (index(child) == UNASSIGNED) visitColorSCCs(child);
            // If the child is not in an SCC
            જો (component(child) == UNASSIGNED) {
                જો (index(vtx) > index(child)) index(vtx) = index(child);
            }
            પાછા ખોટું;
        });

        જો (index(vtx) == મૂળ_Index) {
            // This is the 'root' of an SCC

            // A trivial SCC contains only a single vertex
            અચલ તાર્કિક isTrivial = m_ઢગલો.empty() || index(*m_ઢગલો.back()) < મૂળ_Index;
            // We also need a separate component for vertices that drive themselves (which can
            // happen for input like 'assign a = a'), as we want to extract them (they are cyclic).
            અચલ તાર્કિક drivesSelf = vtx.foreachSink([&](અચલ DfgVertex& sink) {  //
                પાછા &vtx == &sink;
            });

            જો (!isTrivial || drivesSelf) {
                // Allocate new component
                ++m_nonTrivialSCCs;
                component(vtx) = m_nonTrivialSCCs;
                જ્યારે (!m_ઢગલો.empty()) {
                    // Only higher nodes belong to the same SCC
                    જો (index(*m_ઢગલો.back()) < મૂળ_Index) તોડો;
                    component(*m_ઢગલો.back()) = m_nonTrivialSCCs;
                    m_ઢગલો.કાઢો_back();
                }
            } નહીંતર {
                // Trivial SCC (and does not drive itself), so acyclic. Keep it in original graph.
                component(vtx) = 0;
            }
        } નહીંતર {
            // Not the root of an SCC
            m_ઢગલો.ધકેલો_back(&vtx);
        }
    }

    શૂન્ય colorSCCs() {
        // We know constant nodes have no input edges, so they cannot be part
        // of a non-trivial SCC. Mark them as such without any real traversals.
        માટે (અચલ DfgConst& vtx : m_dfg.constVertices()) {
            index(vtx) = 0;
            component(vtx) = 0;
        }

        // We know ast references have no output or input edges
        માટે (અચલ DfgVertexAst& vtx : m_dfg.astVertices()) {
            index(vtx) = 0;
            component(vtx) = 0;
        }

        // Initialize state of variable vertices
        માટે (અચલ DfgVertexVar& vtx : m_dfg.varVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            જો ((!vtx.srcp() && !vtx.defaultp()) || !vtx.hasSinks()) {
                index(vtx) = 0;
                component(vtx) = 0;
                ચાલુ;
            }
            index(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Initialize state of operation vertices
        માટે (અચલ DfgVertex& vtx : m_dfg.opVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            જો (!vtx.nInputs() || !vtx.hasSinks()) {
                index(vtx) = 0;
                component(vtx) = 0;
                ચાલુ;
            }
            index(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Start traversals through not yet visited variables
        માટે (અચલ DfgVertexVar& vtx : m_dfg.varVertices()) {
            જો (index(vtx) == UNASSIGNED) visitColorSCCs(vtx);
        }

        // Start traversals through not yet visited operations
        માટે (અચલ DfgVertex& vtx : m_dfg.opVertices()) {
            જો (index(vtx) == UNASSIGNED) visitColorSCCs(vtx);
        }
    }

    explicit ColorStronglyConnectedComponents(અચલ DfgGraph& dfg, DfgUserMap<uint64_t>& map)
        : m_dfg{dfg}
        , m_map{map} {
        UASSERT(dfg.size() < UNASSIGNED, "Graph too big " << dfg.name());
        // Yet another implementation of Pearce's algorithm.
        colorSCCs();
        // Re-assign mapped values
        m_dfg.forEachVertex([&](અચલ DfgVertex& vtx) {
            અચલ uint64_t c = component(vtx);
            map[vtx] = c;
        });
    }

જાહેર:
    // See declaration of  V3DfgPasses::colorStronglyConnectedComponents
    સ્થિર uint32_t apply(અચલ DfgGraph& dfg, DfgUserMap<uint64_t>& map) {
        પાછા ColorStronglyConnectedComponents{dfg, map}.m_nonTrivialSCCs;
    }
};

uint32_t V3DfgPasses::colorStronglyConnectedComponents(અચલ DfgGraph& dfg,
                                                       DfgUserMap<uint64_t>& map) {
    પાછા ColorStronglyConnectedComponents::apply(dfg, map);
}
