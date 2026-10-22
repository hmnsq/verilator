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

#अन्तर्भाव "V3Dfg.h"
#अन्तर्भाव "V3DfgPasses.h"

#अन्तर्भाव <limits>
#अन्तर्भाव <vector>

वर्ग ColorStronglyConnectedComponents final {
    static_assert(परिमाण(uint32_t[2]) == परिमाण(uint64_t), "Incorrect overlay size");

    // CONSTANTS
    स्थिर constexpr uint32_t UNASSIGNED = std::numeric_limits<uint32_t>::max();

    // STATE
    अचर DfgGraph& m_dfg;  // The input graph
    DfgUserMap<uint64_t>& m_map;  // The result map we are computing - also used for traversal
    uint32_t m_nonTrivialSCCs = 0;  // Number of non-trivial SCCs in the graph
    uint32_t m_index = 0;  // Visitation index counter
    std::vector<अचर DfgVertex*> m_थाक;  // The stack used by the algorithm

    // METHODS
    // Use the bottom 32-bit word as the component number
    uint32_t& component(अचर DfgVertex& vtx) {  //
        फर्काऊ reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[0];
    }
    // Use the top 32-bit word as the visitation index
    uint32_t& index(अचर DfgVertex& vtx) {  //
        फर्काऊ reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[1];
    }

    शून्य visitColorSCCs(अचर DfgVertex& vtx) {
        UDEBUGONLY(UASSERT_OBJ(index(vtx) == UNASSIGNED, &vtx, "Already visited vertex"););

        // Visiting vertex
        अचर माप_प्रकार जरा_Index = index(vtx) = ++m_index;

        // Visit children
        vtx.foreachSink([&](अचर DfgVertex& child) {
            // If the child has not yet been visited, then continue traversal
            यदि (index(child) == UNASSIGNED) visitColorSCCs(child);
            // If the child is not in an SCC
            यदि (component(child) == UNASSIGNED) {
                यदि (index(vtx) > index(child)) index(vtx) = index(child);
            }
            फर्काऊ असत्य;
        });

        यदि (index(vtx) == जरा_Index) {
            // This is the 'root' of an SCC

            // A trivial SCC contains only a single vertex
            अचर तार्किक isTrivial = m_थाक.empty() || index(*m_थाक.back()) < जरा_Index;
            // We also need a separate component for vertices that drive themselves (which can
            // happen for input like 'assign a = a'), as we want to extract them (they are cyclic).
            अचर तार्किक drivesSelf = vtx.foreachSink([&](अचर DfgVertex& sink) {  //
                फर्काऊ &vtx == &sink;
            });

            यदि (!isTrivial || drivesSelf) {
                // Allocate new component
                ++m_nonTrivialSCCs;
                component(vtx) = m_nonTrivialSCCs;
                जबसम्म (!m_थाक.empty()) {
                    // Only higher nodes belong to the same SCC
                    यदि (index(*m_थाक.back()) < जरा_Index) रोक;
                    component(*m_थाक.back()) = m_nonTrivialSCCs;
                    m_थाक.झिक_back();
                }
            } अन्यथा {
                // Trivial SCC (and does not drive itself), so acyclic. Keep it in original graph.
                component(vtx) = 0;
            }
        } अन्यथा {
            // Not the root of an SCC
            m_थाक.थप_back(&vtx);
        }
    }

    शून्य colorSCCs() {
        // We know constant nodes have no input edges, so they cannot be part
        // of a non-trivial SCC. Mark them as such without any real traversals.
        लागि (अचर DfgConst& vtx : m_dfg.constVertices()) {
            index(vtx) = 0;
            component(vtx) = 0;
        }

        // We know ast references have no output or input edges
        लागि (अचर DfgVertexAst& vtx : m_dfg.astVertices()) {
            index(vtx) = 0;
            component(vtx) = 0;
        }

        // Initialize state of variable vertices
        लागि (अचर DfgVertexVar& vtx : m_dfg.varVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            यदि ((!vtx.srcp() && !vtx.defaultp()) || !vtx.hasSinks()) {
                index(vtx) = 0;
                component(vtx) = 0;
                जारी;
            }
            index(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Initialize state of operation vertices
        लागि (अचर DfgVertex& vtx : m_dfg.opVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            यदि (!vtx.nInputs() || !vtx.hasSinks()) {
                index(vtx) = 0;
                component(vtx) = 0;
                जारी;
            }
            index(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Start traversals through not yet visited variables
        लागि (अचर DfgVertexVar& vtx : m_dfg.varVertices()) {
            यदि (index(vtx) == UNASSIGNED) visitColorSCCs(vtx);
        }

        // Start traversals through not yet visited operations
        लागि (अचर DfgVertex& vtx : m_dfg.opVertices()) {
            यदि (index(vtx) == UNASSIGNED) visitColorSCCs(vtx);
        }
    }

    explicit ColorStronglyConnectedComponents(अचर DfgGraph& dfg, DfgUserMap<uint64_t>& map)
        : m_dfg{dfg}
        , m_map{map} {
        UASSERT(dfg.size() < UNASSIGNED, "Graph too big " << dfg.name());
        // Yet another implementation of Pearce's algorithm.
        colorSCCs();
        // Re-assign mapped values
        m_dfg.forEachVertex([&](अचर DfgVertex& vtx) {
            अचर uint64_t c = component(vtx);
            map[vtx] = c;
        });
    }

सार्वजनिक:
    // See declaration of  V3DfgPasses::colorStronglyConnectedComponents
    स्थिर uint32_t apply(अचर DfgGraph& dfg, DfgUserMap<uint64_t>& map) {
        फर्काऊ ColorStronglyConnectedComponents{dfg, map}.m_nonTrivialSCCs;
    }
};

uint32_t V3DfgPasses::colorStronglyConnectedComponents(अचर DfgGraph& dfg,
                                                       DfgUserMap<uint64_t>& map) {
    फर्काऊ ColorStronglyConnectedComponents::apply(dfg, map);
}
