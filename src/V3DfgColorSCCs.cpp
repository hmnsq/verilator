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

#অন্তর্ভাব "V3Dfg.h"
#অন্তর্ভাব "V3DfgPasses.h"

#অন্তর্ভাব <limits>
#অন্তর্ভাব <vector>

বর্গ ColorStronglyConnectedComponents final {
    static_assert(পরিমাণ(uint32_t[2]) == পরিমাণ(uint64_t), "Incorrect overlay size");

    // CONSTANTS
    স্থির constexpr uint32_t UNASSIGNED = std::numeric_limits<uint32_t>::max();

    // STATE
    ধ্রুব DfgGraph& m_dfg;  // The input graph
    DfgUserMap<uint64_t>& m_map;  // The result map we are computing - also used for traversal
    uint32_t m_nonTrivialSCCs = 0;  // Number of non-trivial SCCs in the graph
    uint32_t m_index = 0;  // Visitation index counter
    std::vector<ধ্রুব DfgVertex*> m_স্তূপ;  // The stack used by the algorithm

    // METHODS
    // Use the bottom 32-bit word as the component number
    uint32_t& component(ধ্রুব DfgVertex& vtx) {  //
        প্রত্যাগম reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[0];
    }
    // Use the top 32-bit word as the visitation index
    uint32_t& index(ধ্রুব DfgVertex& vtx) {  //
        প্রত্যাগম reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[1];
    }

    শূন্য visitColorSCCs(ধ্রুব DfgVertex& vtx) {
        UDEBUGONLY(UASSERT_OBJ(index(vtx) == UNASSIGNED, &vtx, "Already visited vertex"););

        // Visiting vertex
        ধ্রুব size_t মূল_Index = index(vtx) = ++m_index;

        // Visit children
        vtx.foreachSink([&](ধ্রুব DfgVertex& child) {
            // If the child has not yet been visited, then continue traversal
            যদি (index(child) == UNASSIGNED) visitColorSCCs(child);
            // If the child is not in an SCC
            যদি (component(child) == UNASSIGNED) {
                যদি (index(vtx) > index(child)) index(vtx) = index(child);
            }
            প্রত্যাগম অসত্য;
        });

        যদি (index(vtx) == মূল_Index) {
            // This is the 'root' of an SCC

            // A trivial SCC contains only a single vertex
            ধ্রুব তার্কিক isTrivial = m_স্তূপ.empty() || index(*m_স্তূপ.back()) < মূল_Index;
            // We also need a separate component for vertices that drive themselves (which can
            // happen for input like 'assign a = a'), as we want to extract them (they are cyclic).
            ধ্রুব তার্কিক drivesSelf = vtx.foreachSink([&](ধ্রুব DfgVertex& sink) {  //
                প্রত্যাগম &vtx == &sink;
            });

            যদি (!isTrivial || drivesSelf) {
                // Allocate new component
                ++m_nonTrivialSCCs;
                component(vtx) = m_nonTrivialSCCs;
                যাবত্ (!m_স্তূপ.empty()) {
                    // Only higher nodes belong to the same SCC
                    যদি (index(*m_স্তূপ.back()) < মূল_Index) ভঙ্গ;
                    component(*m_স্তূপ.back()) = m_nonTrivialSCCs;
                    m_স্তূপ.বের_করো_back();
                }
            } অন্যথা {
                // Trivial SCC (and does not drive itself), so acyclic. Keep it in original graph.
                component(vtx) = 0;
            }
        } অন্যথা {
            // Not the root of an SCC
            m_স্তূপ.ঠেলো_back(&vtx);
        }
    }

    শূন্য colorSCCs() {
        // We know constant nodes have no input edges, so they cannot be part
        // of a non-trivial SCC. Mark them as such without any real traversals.
        প্রতি (ধ্রুব DfgConst& vtx : m_dfg.constVertices()) {
            index(vtx) = 0;
            component(vtx) = 0;
        }

        // We know ast references have no output or input edges
        প্রতি (ধ্রুব DfgVertexAst& vtx : m_dfg.astVertices()) {
            index(vtx) = 0;
            component(vtx) = 0;
        }

        // Initialize state of variable vertices
        প্রতি (ধ্রুব DfgVertexVar& vtx : m_dfg.varVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            যদি ((!vtx.srcp() && !vtx.defaultp()) || !vtx.hasSinks()) {
                index(vtx) = 0;
                component(vtx) = 0;
                অনুবর্ত;
            }
            index(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Initialize state of operation vertices
        প্রতি (ধ্রুব DfgVertex& vtx : m_dfg.opVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            যদি (!vtx.nInputs() || !vtx.hasSinks()) {
                index(vtx) = 0;
                component(vtx) = 0;
                অনুবর্ত;
            }
            index(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Start traversals through not yet visited variables
        প্রতি (ধ্রুব DfgVertexVar& vtx : m_dfg.varVertices()) {
            যদি (index(vtx) == UNASSIGNED) visitColorSCCs(vtx);
        }

        // Start traversals through not yet visited operations
        প্রতি (ধ্রুব DfgVertex& vtx : m_dfg.opVertices()) {
            যদি (index(vtx) == UNASSIGNED) visitColorSCCs(vtx);
        }
    }

    explicit ColorStronglyConnectedComponents(ধ্রুব DfgGraph& dfg, DfgUserMap<uint64_t>& map)
        : m_dfg{dfg}
        , m_map{map} {
        UASSERT(dfg.size() < UNASSIGNED, "Graph too big " << dfg.name());
        // Yet another implementation of Pearce's algorithm.
        colorSCCs();
        // Re-assign mapped values
        m_dfg.forEachVertex([&](ধ্রুব DfgVertex& vtx) {
            ধ্রুব uint64_t c = component(vtx);
            map[vtx] = c;
        });
    }

সার্বজনিক:
    // See declaration of  V3DfgPasses::colorStronglyConnectedComponents
    স্থির uint32_t apply(ধ্রুব DfgGraph& dfg, DfgUserMap<uint64_t>& map) {
        প্রত্যাগম ColorStronglyConnectedComponents{dfg, map}.m_nonTrivialSCCs;
    }
};

uint32_t V3DfgPasses::colorStronglyConnectedComponents(ধ্রুব DfgGraph& dfg,
                                                       DfgUserMap<uint64_t>& map) {
    প্রত্যাগম ColorStronglyConnectedComponents::apply(dfg, map);
}
