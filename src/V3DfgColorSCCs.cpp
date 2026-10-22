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

#समावेश "V3Dfg.h"
#समावेश "V3DfgPasses.h"

#समावेश <limits>
#समावेश <vector>

श्रेणी ColorStronglyConnectedComponents final {
    static_assert(माप(uint32_t[2]) == माप(uint64_t), "Incorrect overlay size");

    // CONSTANTS
    अटल constexpr uint32_t UNASSIGNED = std::numeric_limits<uint32_t>::max();

    // STATE
    स्थिर DfgGraph& m_dfg;  // The input graph
    DfgUserMap<uint64_t>& m_map;  // The result map we are computing - also used for traversal
    uint32_t m_nonTrivialSCCs = 0;  // Number of non-trivial SCCs in the graph
    uint32_t m_सूचकांक = 0;  // Visitation index counter
    std::vector<स्थिर DfgVertex*> m_ढेर;  // The stack used by the algorithm

    // METHODS
    // Use the bottom 32-bit word as the component number
    uint32_t& component(स्थिर DfgVertex& vtx) {  //
        वापस reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[0];
    }
    // Use the top 32-bit word as the visitation index
    uint32_t& सूचकांक_2(स्थिर DfgVertex& vtx) {  //
        वापस reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[1];
    }

    व्योम visitColorSCCs(स्थिर DfgVertex& vtx) {
        UDEBUGONLY(UASSERT_OBJ(सूचकांक_2(vtx) == UNASSIGNED, &vtx, "Already visited vertex"););

        // Visiting vertex
        स्थिर माप_प्रकार मूल_सूचकांक = सूचकांक_2(vtx) = ++m_सूचकांक;

        // Visit children
        vtx.foreachSink([&](स्थिर DfgVertex& child) {
            // If the child has not yet been visited, then continue traversal
            अगर (सूचकांक_2(child) == UNASSIGNED) visitColorSCCs(child);
            // If the child is not in an SCC
            अगर (component(child) == UNASSIGNED) {
                अगर (सूचकांक_2(vtx) > सूचकांक_2(child)) सूचकांक_2(vtx) = सूचकांक_2(child);
            }
            वापस false;
        });

        अगर (सूचकांक_2(vtx) == मूल_सूचकांक) {
            // This is the 'root' of an SCC

            // A trivial SCC contains only a single vertex
            स्थिर bool isTrivial = m_ढेर.empty() || सूचकांक_2(*m_ढेर.back()) < मूल_सूचकांक;
            // We also need a separate component for vertices that drive themselves (which can
            // happen for input like 'assign a = a'), as we want to extract them (they are cyclic).
            स्थिर bool drivesSelf = vtx.foreachSink([&](स्थिर DfgVertex& sink) {  //
                वापस &vtx == &sink;
            });

            अगर (!isTrivial || drivesSelf) {
                // Allocate new component
                ++m_nonTrivialSCCs;
                component(vtx) = m_nonTrivialSCCs;
                जबतक (!m_ढेर.empty()) {
                    // Only higher nodes belong to the same SCC
                    अगर (सूचकांक_2(*m_ढेर.back()) < मूल_सूचकांक) अवरोध;
                    component(*m_ढेर.back()) = m_nonTrivialSCCs;
                    m_ढेर.निकालो_back();
                }
            } अन्यथा {
                // Trivial SCC (and does not drive itself), so acyclic. Keep it in original graph.
                component(vtx) = 0;
            }
        } अन्यथा {
            // Not the root of an SCC
            m_ढेर.धकेलो_back(&vtx);
        }
    }

    व्योम colorSCCs() {
        // We know constant nodes have no input edges, so they cannot be part
        // of a non-trivial SCC. Mark them as such without any real traversals.
        क्रम (स्थिर DfgConst& vtx : m_dfg.constVertices()) {
            सूचकांक_2(vtx) = 0;
            component(vtx) = 0;
        }

        // We know ast references have no output or input edges
        क्रम (स्थिर DfgVertexAst& vtx : m_dfg.astVertices()) {
            सूचकांक_2(vtx) = 0;
            component(vtx) = 0;
        }

        // Initialize state of variable vertices
        क्रम (स्थिर DfgVertexVar& vtx : m_dfg.varVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            अगर ((!vtx.srcp() && !vtx.defaultp()) || !vtx.hasSinks()) {
                सूचकांक_2(vtx) = 0;
                component(vtx) = 0;
                जारी;
            }
            सूचकांक_2(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Initialize state of operation vertices
        क्रम (स्थिर DfgVertex& vtx : m_dfg.opVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            अगर (!vtx.nInputs() || !vtx.hasSinks()) {
                सूचकांक_2(vtx) = 0;
                component(vtx) = 0;
                जारी;
            }
            सूचकांक_2(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Start traversals through not yet visited variables
        क्रम (स्थिर DfgVertexVar& vtx : m_dfg.varVertices()) {
            अगर (सूचकांक_2(vtx) == UNASSIGNED) visitColorSCCs(vtx);
        }

        // Start traversals through not yet visited operations
        क्रम (स्थिर DfgVertex& vtx : m_dfg.opVertices()) {
            अगर (सूचकांक_2(vtx) == UNASSIGNED) visitColorSCCs(vtx);
        }
    }

    explicit ColorStronglyConnectedComponents(स्थिर DfgGraph& dfg, DfgUserMap<uint64_t>& map)
        : m_dfg{dfg}
        , m_map{map} {
        UASSERT(dfg.आकार() < UNASSIGNED, "Graph too big " << dfg.name());
        // Yet another implementation of Pearce's algorithm.
        colorSCCs();
        // Re-assign mapped values
        m_dfg.forEachVertex([&](स्थिर DfgVertex& vtx) {
            स्थिर uint64_t c = component(vtx);
            map[vtx] = c;
        });
    }

खुला:
    // See declaration of  V3DfgPasses::colorStronglyConnectedComponents
    अटल uint32_t apply(स्थिर DfgGraph& dfg, DfgUserMap<uint64_t>& map) {
        वापस ColorStronglyConnectedComponents{dfg, map}.m_nonTrivialSCCs;
    }
};

uint32_t V3DfgPasses::colorStronglyConnectedComponents(स्थिर DfgGraph& dfg,
                                                       DfgUserMap<uint64_t>& map) {
    वापस ColorStronglyConnectedComponents::apply(dfg, map);
}
