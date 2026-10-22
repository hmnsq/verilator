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

#ଅନ୍ତର୍ଭାଵ "V3Dfg.h"
#ଅନ୍ତର୍ଭାଵ "V3DfgPasses.h"

#ଅନ୍ତର୍ଭାଵ <limits>
#ଅନ୍ତର୍ଭାଵ <vector>

ଶ୍ରେଣୀ ColorStronglyConnectedComponents final {
    static_assert(ଆକାର(uint32_t[2]) == ଆକାର(uint64_t), "Incorrect overlay size");

    // CONSTANTS
    ସ୍ଥିର constexpr uint32_t UNASSIGNED = std::numeric_limits<uint32_t>::max();

    // STATE
    ଅଚଳ DfgGraph& m_dfg;  // The input graph
    DfgUserMap<uint64_t>& m_map;  // The result map we are computing - also used for traversal
    uint32_t m_nonTrivialSCCs = 0;  // Number of non-trivial SCCs in the graph
    uint32_t m_index = 0;  // Visitation index counter
    std::vector<ଅଚଳ DfgVertex*> m_ଗଦା;  // The stack used by the algorithm

    // METHODS
    // Use the bottom 32-bit word as the component number
    uint32_t& component(ଅଚଳ DfgVertex& vtx) {  //
        ଫେରନ୍ତୁ reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[0];
    }
    // Use the top 32-bit word as the visitation index
    uint32_t& index(ଅଚଳ DfgVertex& vtx) {  //
        ଫେରନ୍ତୁ reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[1];
    }

    ଶୂନ୍ୟ visitColorSCCs(ଅଚଳ DfgVertex& vtx) {
        UDEBUGONLY(UASSERT_OBJ(index(vtx) == UNASSIGNED, &vtx, "Already visited vertex"););

        // Visiting vertex
        ଅଚଳ size_t ମୂଳ_Index = index(vtx) = ++m_index;

        // Visit children
        vtx.foreachSink([&](ଅଚଳ DfgVertex& child) {
            // If the child has not yet been visited, then continue traversal
            ଯଦି (index(child) == UNASSIGNED) visitColorSCCs(child);
            // If the child is not in an SCC
            ଯଦି (component(child) == UNASSIGNED) {
                ଯଦି (index(vtx) > index(child)) index(vtx) = index(child);
            }
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });

        ଯଦି (index(vtx) == ମୂଳ_Index) {
            // This is the 'root' of an SCC

            // A trivial SCC contains only a single vertex
            ଅଚଳ ତାର୍କିକ isTrivial = m_ଗଦା.empty() || index(*m_ଗଦା.back()) < ମୂଳ_Index;
            // We also need a separate component for vertices that drive themselves (which can
            // happen for input like 'assign a = a'), as we want to extract them (they are cyclic).
            ଅଚଳ ତାର୍କିକ drivesSelf = vtx.foreachSink([&](ଅଚଳ DfgVertex& sink) {  //
                ଫେରନ୍ତୁ &vtx == &sink;
            });

            ଯଦି (!isTrivial || drivesSelf) {
                // Allocate new component
                ++m_nonTrivialSCCs;
                component(vtx) = m_nonTrivialSCCs;
                ଯେତେବେଳେ (!m_ଗଦା.empty()) {
                    // Only higher nodes belong to the same SCC
                    ଯଦି (index(*m_ଗଦା.back()) < ମୂଳ_Index) ଭାଙ୍ଗ;
                    component(*m_ଗଦା.back()) = m_nonTrivialSCCs;
                    m_ଗଦା.କାଢ଼_back();
                }
            } ଅନ୍ୟଥା {
                // Trivial SCC (and does not drive itself), so acyclic. Keep it in original graph.
                component(vtx) = 0;
            }
        } ଅନ୍ୟଥା {
            // Not the root of an SCC
            m_ଗଦା.ଠେଲ_back(&vtx);
        }
    }

    ଶୂନ୍ୟ colorSCCs() {
        // We know constant nodes have no input edges, so they cannot be part
        // of a non-trivial SCC. Mark them as such without any real traversals.
        ପାଇଁ (ଅଚଳ DfgConst& vtx : m_dfg.constVertices()) {
            index(vtx) = 0;
            component(vtx) = 0;
        }

        // We know ast references have no output or input edges
        ପାଇଁ (ଅଚଳ DfgVertexAst& vtx : m_dfg.astVertices()) {
            index(vtx) = 0;
            component(vtx) = 0;
        }

        // Initialize state of variable vertices
        ପାଇଁ (ଅଚଳ DfgVertexVar& vtx : m_dfg.varVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            ଯଦି ((!vtx.srcp() && !vtx.defaultp()) || !vtx.hasSinks()) {
                index(vtx) = 0;
                component(vtx) = 0;
                ଜାରି;
            }
            index(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Initialize state of operation vertices
        ପାଇଁ (ଅଚଳ DfgVertex& vtx : m_dfg.opVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            ଯଦି (!vtx.nInputs() || !vtx.hasSinks()) {
                index(vtx) = 0;
                component(vtx) = 0;
                ଜାରି;
            }
            index(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Start traversals through not yet visited variables
        ପାଇଁ (ଅଚଳ DfgVertexVar& vtx : m_dfg.varVertices()) {
            ଯଦି (index(vtx) == UNASSIGNED) visitColorSCCs(vtx);
        }

        // Start traversals through not yet visited operations
        ପାଇଁ (ଅଚଳ DfgVertex& vtx : m_dfg.opVertices()) {
            ଯଦି (index(vtx) == UNASSIGNED) visitColorSCCs(vtx);
        }
    }

    explicit ColorStronglyConnectedComponents(ଅଚଳ DfgGraph& dfg, DfgUserMap<uint64_t>& map)
        : m_dfg{dfg}
        , m_map{map} {
        UASSERT(dfg.size() < UNASSIGNED, "Graph too big " << dfg.name());
        // Yet another implementation of Pearce's algorithm.
        colorSCCs();
        // Re-assign mapped values
        m_dfg.forEachVertex([&](ଅଚଳ DfgVertex& vtx) {
            ଅଚଳ uint64_t c = component(vtx);
            map[vtx] = c;
        });
    }

ସର୍ବସାଧାରଣ:
    // See declaration of  V3DfgPasses::colorStronglyConnectedComponents
    ସ୍ଥିର uint32_t apply(ଅଚଳ DfgGraph& dfg, DfgUserMap<uint64_t>& map) {
        ଫେରନ୍ତୁ ColorStronglyConnectedComponents{dfg, map}.m_nonTrivialSCCs;
    }
};

uint32_t V3DfgPasses::colorStronglyConnectedComponents(ଅଚଳ DfgGraph& dfg,
                                                       DfgUserMap<uint64_t>& map) {
    ଫେରନ୍ତୁ ColorStronglyConnectedComponents::apply(dfg, map);
}
