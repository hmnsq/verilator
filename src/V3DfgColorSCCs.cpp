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

#அந்தர்பாவ "V3Dfg.h"
#அந்தர்பாவ "V3DfgPasses.h"

#அந்தர்பாவ <limits>
#அந்தர்பாவ <vector>

வகுப்பு ColorStronglyConnectedComponents final {
    static_assert(அளவு(uint32_t[2]) == அளவு(uint64_t), "Incorrect overlay size");

    // CONSTANTS
    நிலையான constexpr uint32_t UNASSIGNED = std::numeric_limits<uint32_t>::max();

    // STATE
    மாறிலி DfgGraph& m_dfg;  // The input graph
    DfgUserMap<uint64_t>& m_map;  // The result map we are computing - also used for traversal
    uint32_t m_nonTrivialSCCs = 0;  // Number of non-trivial SCCs in the graph
    uint32_t m_index = 0;  // Visitation index counter
    std::vector<மாறிலி DfgVertex*> m_அடுக்கு;  // The stack used by the algorithm

    // METHODS
    // Use the bottom 32-bit word as the component number
    uint32_t& component(மாறிலி DfgVertex& vtx) {  //
        திரும்பு reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[0];
    }
    // Use the top 32-bit word as the visitation index
    uint32_t& index(மாறிலி DfgVertex& vtx) {  //
        திரும்பு reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[1];
    }

    வெறுமை visitColorSCCs(மாறிலி DfgVertex& vtx) {
        UDEBUGONLY(UASSERT_OBJ(index(vtx) == UNASSIGNED, &vtx, "Already visited vertex"););

        // Visiting vertex
        மாறிலி size_t வேர்_Index = index(vtx) = ++m_index;

        // Visit children
        vtx.foreachSink([&](மாறிலி DfgVertex& child) {
            // If the child has not yet been visited, then continue traversal
            எனில் (index(child) == UNASSIGNED) visitColorSCCs(child);
            // If the child is not in an SCC
            எனில் (component(child) == UNASSIGNED) {
                எனில் (index(vtx) > index(child)) index(vtx) = index(child);
            }
            திரும்பு பொய்;
        });

        எனில் (index(vtx) == வேர்_Index) {
            // This is the 'root' of an SCC

            // A trivial SCC contains only a single vertex
            மாறிலி தருக்க isTrivial = m_அடுக்கு.empty() || index(*m_அடுக்கு.back()) < வேர்_Index;
            // We also need a separate component for vertices that drive themselves (which can
            // happen for input like 'assign a = a'), as we want to extract them (they are cyclic).
            மாறிலி தருக்க drivesSelf = vtx.foreachSink([&](மாறிலி DfgVertex& sink) {  //
                திரும்பு &vtx == &sink;
            });

            எனில் (!isTrivial || drivesSelf) {
                // Allocate new component
                ++m_nonTrivialSCCs;
                component(vtx) = m_nonTrivialSCCs;
                போது (!m_அடுக்கு.empty()) {
                    // Only higher nodes belong to the same SCC
                    எனில் (index(*m_அடுக்கு.back()) < வேர்_Index) நிறுத்து;
                    component(*m_அடுக்கு.back()) = m_nonTrivialSCCs;
                    m_அடுக்கு.எடு_back();
                }
            } இல்லையெனில் {
                // Trivial SCC (and does not drive itself), so acyclic. Keep it in original graph.
                component(vtx) = 0;
            }
        } இல்லையெனில் {
            // Not the root of an SCC
            m_அடுக்கு.தள்ளு_back(&vtx);
        }
    }

    வெறுமை colorSCCs() {
        // We know constant nodes have no input edges, so they cannot be part
        // of a non-trivial SCC. Mark them as such without any real traversals.
        க்கு (மாறிலி DfgConst& vtx : m_dfg.constVertices()) {
            index(vtx) = 0;
            component(vtx) = 0;
        }

        // We know ast references have no output or input edges
        க்கு (மாறிலி DfgVertexAst& vtx : m_dfg.astVertices()) {
            index(vtx) = 0;
            component(vtx) = 0;
        }

        // Initialize state of variable vertices
        க்கு (மாறிலி DfgVertexVar& vtx : m_dfg.varVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            எனில் ((!vtx.srcp() && !vtx.defaultp()) || !vtx.hasSinks()) {
                index(vtx) = 0;
                component(vtx) = 0;
                தொடர்;
            }
            index(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Initialize state of operation vertices
        க்கு (மாறிலி DfgVertex& vtx : m_dfg.opVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            எனில் (!vtx.nInputs() || !vtx.hasSinks()) {
                index(vtx) = 0;
                component(vtx) = 0;
                தொடர்;
            }
            index(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Start traversals through not yet visited variables
        க்கு (மாறிலி DfgVertexVar& vtx : m_dfg.varVertices()) {
            எனில் (index(vtx) == UNASSIGNED) visitColorSCCs(vtx);
        }

        // Start traversals through not yet visited operations
        க்கு (மாறிலி DfgVertex& vtx : m_dfg.opVertices()) {
            எனில் (index(vtx) == UNASSIGNED) visitColorSCCs(vtx);
        }
    }

    explicit ColorStronglyConnectedComponents(மாறிலி DfgGraph& dfg, DfgUserMap<uint64_t>& map)
        : m_dfg{dfg}
        , m_map{map} {
        UASSERT(dfg.size() < UNASSIGNED, "Graph too big " << dfg.name());
        // Yet another implementation of Pearce's algorithm.
        colorSCCs();
        // Re-assign mapped values
        m_dfg.forEachVertex([&](மாறிலி DfgVertex& vtx) {
            மாறிலி uint64_t c = component(vtx);
            map[vtx] = c;
        });
    }

பொது:
    // See declaration of  V3DfgPasses::colorStronglyConnectedComponents
    நிலையான uint32_t apply(மாறிலி DfgGraph& dfg, DfgUserMap<uint64_t>& map) {
        திரும்பு ColorStronglyConnectedComponents{dfg, map}.m_nonTrivialSCCs;
    }
};

uint32_t V3DfgPasses::colorStronglyConnectedComponents(மாறிலி DfgGraph& dfg,
                                                       DfgUserMap<uint64_t>& map) {
    திரும்பு ColorStronglyConnectedComponents::apply(dfg, map);
}
