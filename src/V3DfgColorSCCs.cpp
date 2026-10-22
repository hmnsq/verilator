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

#അന്തര്ഭാവ "V3Dfg.h"
#അന്തര്ഭാവ "V3DfgPasses.h"

#അന്തര്ഭാവ <limits>
#അന്തര്ഭാവ <vector>

വർഗ്ഗം ColorStronglyConnectedComponents final {
    static_assert(വലുപ്പം(uint32_t[2]) == വലുപ്പം(uint64_t), "Incorrect overlay size");

    // CONSTANTS
    സ്ഥിതം constexpr uint32_t UNASSIGNED = std::numeric_limits<uint32_t>::max();

    // STATE
    സ്ഥിരം DfgGraph& m_dfg;  // The input graph
    DfgUserMap<uint64_t>& m_map;  // The result map we are computing - also used for traversal
    uint32_t m_nonTrivialSCCs = 0;  // Number of non-trivial SCCs in the graph
    uint32_t m_index = 0;  // Visitation index counter
    std::vector<സ്ഥിരം DfgVertex*> m_അട്ടി;  // The stack used by the algorithm

    // METHODS
    // Use the bottom 32-bit word as the component number
    uint32_t& component(സ്ഥിരം DfgVertex& vtx) {  //
        തിരികെ reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[0];
    }
    // Use the top 32-bit word as the visitation index
    uint32_t& index(സ്ഥിരം DfgVertex& vtx) {  //
        തിരികെ reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[1];
    }

    ശൂന്യം visitColorSCCs(സ്ഥിരം DfgVertex& vtx) {
        UDEBUGONLY(UASSERT_OBJ(index(vtx) == UNASSIGNED, &vtx, "Already visited vertex"););

        // Visiting vertex
        സ്ഥിരം size_t വേര്_Index = index(vtx) = ++m_index;

        // Visit children
        vtx.foreachSink([&](സ്ഥിരം DfgVertex& child) {
            // If the child has not yet been visited, then continue traversal
            എങ്കിൽ (index(child) == UNASSIGNED) visitColorSCCs(child);
            // If the child is not in an SCC
            എങ്കിൽ (component(child) == UNASSIGNED) {
                എങ്കിൽ (index(vtx) > index(child)) index(vtx) = index(child);
            }
            തിരികെ അസത്യം;
        });

        എങ്കിൽ (index(vtx) == വേര്_Index) {
            // This is the 'root' of an SCC

            // A trivial SCC contains only a single vertex
            സ്ഥിരം തർക്കം isTrivial = m_അട്ടി.empty() || index(*m_അട്ടി.back()) < വേര്_Index;
            // We also need a separate component for vertices that drive themselves (which can
            // happen for input like 'assign a = a'), as we want to extract them (they are cyclic).
            സ്ഥിരം തർക്കം drivesSelf = vtx.foreachSink([&](സ്ഥിരം DfgVertex& sink) {  //
                തിരികെ &vtx == &sink;
            });

            എങ്കിൽ (!isTrivial || drivesSelf) {
                // Allocate new component
                ++m_nonTrivialSCCs;
                component(vtx) = m_nonTrivialSCCs;
                ആകുമ്പോൾ (!m_അട്ടി.empty()) {
                    // Only higher nodes belong to the same SCC
                    എങ്കിൽ (index(*m_അട്ടി.back()) < വേര്_Index) നിർത്തുക;
                    component(*m_അട്ടി.back()) = m_nonTrivialSCCs;
                    m_അട്ടി.എടുക്കുക_back();
                }
            } അല്ലെങ്കിൽ {
                // Trivial SCC (and does not drive itself), so acyclic. Keep it in original graph.
                component(vtx) = 0;
            }
        } അല്ലെങ്കിൽ {
            // Not the root of an SCC
            m_അട്ടി.തള്ളുക_back(&vtx);
        }
    }

    ശൂന്യം colorSCCs() {
        // We know constant nodes have no input edges, so they cannot be part
        // of a non-trivial SCC. Mark them as such without any real traversals.
        വേണ്ടി (സ്ഥിരം DfgConst& vtx : m_dfg.constVertices()) {
            index(vtx) = 0;
            component(vtx) = 0;
        }

        // We know ast references have no output or input edges
        വേണ്ടി (സ്ഥിരം DfgVertexAst& vtx : m_dfg.astVertices()) {
            index(vtx) = 0;
            component(vtx) = 0;
        }

        // Initialize state of variable vertices
        വേണ്ടി (സ്ഥിരം DfgVertexVar& vtx : m_dfg.varVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            എങ്കിൽ ((!vtx.srcp() && !vtx.defaultp()) || !vtx.hasSinks()) {
                index(vtx) = 0;
                component(vtx) = 0;
                തുടരുക;
            }
            index(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Initialize state of operation vertices
        വേണ്ടി (സ്ഥിരം DfgVertex& vtx : m_dfg.opVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            എങ്കിൽ (!vtx.nInputs() || !vtx.hasSinks()) {
                index(vtx) = 0;
                component(vtx) = 0;
                തുടരുക;
            }
            index(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Start traversals through not yet visited variables
        വേണ്ടി (സ്ഥിരം DfgVertexVar& vtx : m_dfg.varVertices()) {
            എങ്കിൽ (index(vtx) == UNASSIGNED) visitColorSCCs(vtx);
        }

        // Start traversals through not yet visited operations
        വേണ്ടി (സ്ഥിരം DfgVertex& vtx : m_dfg.opVertices()) {
            എങ്കിൽ (index(vtx) == UNASSIGNED) visitColorSCCs(vtx);
        }
    }

    explicit ColorStronglyConnectedComponents(സ്ഥിരം DfgGraph& dfg, DfgUserMap<uint64_t>& map)
        : m_dfg{dfg}
        , m_map{map} {
        UASSERT(dfg.size() < UNASSIGNED, "Graph too big " << dfg.name());
        // Yet another implementation of Pearce's algorithm.
        colorSCCs();
        // Re-assign mapped values
        m_dfg.forEachVertex([&](സ്ഥിരം DfgVertex& vtx) {
            സ്ഥിരം uint64_t c = component(vtx);
            map[vtx] = c;
        });
    }

പൊതു:
    // See declaration of  V3DfgPasses::colorStronglyConnectedComponents
    സ്ഥിതം uint32_t apply(സ്ഥിരം DfgGraph& dfg, DfgUserMap<uint64_t>& map) {
        തിരികെ ColorStronglyConnectedComponents{dfg, map}.m_nonTrivialSCCs;
    }
};

uint32_t V3DfgPasses::colorStronglyConnectedComponents(സ്ഥിരം DfgGraph& dfg,
                                                       DfgUserMap<uint64_t>& map) {
    തിരികെ ColorStronglyConnectedComponents::apply(dfg, map);
}
