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

#ضمن "V3Dfg.h"
#ضمن "V3DfgPasses.h"

#ضمن <limits>
#ضمن <vector>

מחלקה ColorStronglyConnectedComponents final {
    static_assert(حجم(uint32_t[2]) == حجم(uint64_t), "Incorrect overlay size");

    // CONSTANTS
    ساكن constexpr uint32_t UNASSIGNED = std::numeric_limits<uint32_t>::max();

    // STATE
    ثابت DfgGraph& m_dfg;  // The input graph
    DfgUserMap<uint64_t>& m_map;  // The result map we are computing - also used for traversal
    uint32_t m_nonTrivialSCCs = 0;  // Number of non-trivial SCCs in the graph
    uint32_t m_index = 0;  // Visitation index counter
    std::vector<ثابت DfgVertex*> m_stack;  // The stack used by the algorithm

    // METHODS
    // Use the bottom 32-bit word as the component number
    uint32_t& component(ثابت DfgVertex& vtx) {  //
        החזר reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[0];
    }
    // Use the top 32-bit word as the visitation index
    uint32_t& index(ثابت DfgVertex& vtx) {  //
        החזר reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[1];
    }

    فراغ visitColorSCCs(ثابت DfgVertex& vtx) {
        UDEBUGONLY(UASSERT_OBJ(index(vtx) == UNASSIGNED, &vtx, "Already visited vertex"););

        // Visiting vertex
        ثابت size_t rootIndex = index(vtx) = ++m_index;

        // Visit children
        vtx.foreachSink([&](ثابت DfgVertex& child) {
            // If the child has not yet been visited, then continue traversal
            אם (index(child) == UNASSIGNED) visitColorSCCs(child);
            // If the child is not in an SCC
            אם (component(child) == UNASSIGNED) {
                אם (index(vtx) > index(child)) index(vtx) = index(child);
            }
            החזר שקר;
        });

        אם (index(vtx) == rootIndex) {
            // This is the 'root' of an SCC

            // A trivial SCC contains only a single vertex
            ثابت منطقي isTrivial = m_stack.empty() || index(*m_stack.back()) < rootIndex;
            // We also need a separate component for vertices that drive themselves (which can
            // happen for input like 'assign a = a'), as we want to extract them (they are cyclic).
            ثابت منطقي drivesSelf = vtx.foreachSink([&](ثابت DfgVertex& sink) {  //
                החזר &vtx == &sink;
            });

            אם (!isTrivial || drivesSelf) {
                // Allocate new component
                ++m_nonTrivialSCCs;
                component(vtx) = m_nonTrivialSCCs;
                כלעוד (!m_stack.empty()) {
                    // Only higher nodes belong to the same SCC
                    אם (index(*m_stack.back()) < rootIndex) اكسر;
                    component(*m_stack.back()) = m_nonTrivialSCCs;
                    m_stack.pop_back();
                }
            } אחרת {
                // Trivial SCC (and does not drive itself), so acyclic. Keep it in original graph.
                component(vtx) = 0;
            }
        } אחרת {
            // Not the root of an SCC
            m_stack.push_back(&vtx);
        }
    }

    فراغ colorSCCs() {
        // We know constant nodes have no input edges, so they cannot be part
        // of a non-trivial SCC. Mark them as such without any real traversals.
        עבור (ثابت DfgConst& vtx : m_dfg.constVertices()) {
            index(vtx) = 0;
            component(vtx) = 0;
        }

        // We know ast references have no output or input edges
        עבור (ثابت DfgVertexAst& vtx : m_dfg.astVertices()) {
            index(vtx) = 0;
            component(vtx) = 0;
        }

        // Initialize state of variable vertices
        עבור (ثابت DfgVertexVar& vtx : m_dfg.varVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            אם ((!vtx.srcp() && !vtx.defaultp()) || !vtx.hasSinks()) {
                index(vtx) = 0;
                component(vtx) = 0;
                استمر;
            }
            index(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Initialize state of operation vertices
        עבור (ثابت DfgVertex& vtx : m_dfg.opVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            אם (!vtx.nInputs() || !vtx.hasSinks()) {
                index(vtx) = 0;
                component(vtx) = 0;
                استمر;
            }
            index(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Start traversals through not yet visited variables
        עבור (ثابت DfgVertexVar& vtx : m_dfg.varVertices()) {
            אם (index(vtx) == UNASSIGNED) visitColorSCCs(vtx);
        }

        // Start traversals through not yet visited operations
        עבור (ثابت DfgVertex& vtx : m_dfg.opVertices()) {
            אם (index(vtx) == UNASSIGNED) visitColorSCCs(vtx);
        }
    }

    explicit ColorStronglyConnectedComponents(ثابت DfgGraph& dfg, DfgUserMap<uint64_t>& map)
        : m_dfg{dfg}
        , m_map{map} {
        UASSERT(dfg.size() < UNASSIGNED, "Graph too big " << dfg.name());
        // Yet another implementation of Pearce's algorithm.
        colorSCCs();
        // Re-assign mapped values
        m_dfg.forEachVertex([&](ثابت DfgVertex& vtx) {
            ثابت uint64_t c = component(vtx);
            map[vtx] = c;
        });
    }

عام:
    // See declaration of  V3DfgPasses::colorStronglyConnectedComponents
    ساكن uint32_t apply(ثابت DfgGraph& dfg, DfgUserMap<uint64_t>& map) {
        החזר ColorStronglyConnectedComponents{dfg, map}.m_nonTrivialSCCs;
    }
};

uint32_t V3DfgPasses::colorStronglyConnectedComponents(ثابت DfgGraph& dfg,
                                                       DfgUserMap<uint64_t>& map) {
    החזר ColorStronglyConnectedComponents::apply(dfg, map);
}
