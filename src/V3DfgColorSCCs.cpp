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

শ্রেণী ColorStronglyConnectedComponents final {
    static_assert(আকার(uint32_t[2]) == আকার(uint64_t), "Incorrect overlay size");

    // CONSTANTS
    স্থির constexpr uint32_t UNASSIGNED = std::numeric_limits<uint32_t>::max();

    // STATE
    ধ্রুবক DfgGraph& m_dfg;  // The input graph
    DfgUserMap<uint64_t>& m_map;  // The result map we are computing - also used for traversal
    uint32_t m_nonTrivialSCCs = 0;  // Number of non-trivial SCCs in the graph
    uint32_t m_সূচক = 0;  // Visitation index counter
    std::vector<ধ্রুবক DfgVertex*> m_স্তূপ;  // The stack used by the algorithm

    // METHODS
    // Use the bottom 32-bit word as the component number
    uint32_t& component(ধ্রুবক DfgVertex& vtx) {  //
        ফেরত reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[0];
    }
    // Use the top 32-bit word as the visitation index
    uint32_t& সূচক(ধ্রুবক DfgVertex& vtx) {  //
        ফেরত reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[1];
    }

    শূন্য visitColorSCCs(ধ্রুবক DfgVertex& vtx) {
        UDEBUGONLY(UASSERT_OBJ(সূচক(vtx) == UNASSIGNED, &vtx, "Already visited vertex"););

        // Visiting vertex
        ধ্রুবক আকার_t মূল_সূচক = সূচক(vtx) = ++m_সূচক;

        // Visit children
        vtx.foreachSink([&](ধ্রুবক DfgVertex& child) {
            // If the child has not yet been visited, then continue traversal
            যদি (সূচক(child) == UNASSIGNED) visitColorSCCs(child);
            // If the child is not in an SCC
            যদি (component(child) == UNASSIGNED) {
                যদি (সূচক(vtx) > সূচক(child)) সূচক(vtx) = সূচক(child);
            }
            ফেরত মিথ্যা;
        });

        যদি (সূচক(vtx) == মূল_সূচক) {
            // This is the 'root' of an SCC

            // A trivial SCC contains only a single vertex
            ধ্রুবক যৌক্তিক isTrivial = m_স্তূপ.empty() || সূচক(*m_স্তূপ.back()) < মূল_সূচক;
            // We also need a separate component for vertices that drive themselves (which can
            // happen for input like 'assign a = a'), as we want to extract them (they are cyclic).
            ধ্রুবক যৌক্তিক drivesSelf = vtx.foreachSink([&](ধ্রুবক DfgVertex& sink) {  //
                ফেরত &vtx == &sink;
            });

            যদি (!isTrivial || drivesSelf) {
                // Allocate new component
                ++m_nonTrivialSCCs;
                component(vtx) = m_nonTrivialSCCs;
                যখন (!m_স্তূপ.empty()) {
                    // Only higher nodes belong to the same SCC
                    যদি (সূচক(*m_স্তূপ.back()) < মূল_সূচক) বিরতি;
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
        জন্য (ধ্রুবক DfgConst& vtx : m_dfg.constVertices()) {
            সূচক(vtx) = 0;
            component(vtx) = 0;
        }

        // We know ast references have no output or input edges
        জন্য (ধ্রুবক DfgVertexAst& vtx : m_dfg.astVertices()) {
            সূচক(vtx) = 0;
            component(vtx) = 0;
        }

        // Initialize state of variable vertices
        জন্য (ধ্রুবক DfgVertexVar& vtx : m_dfg.varVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            যদি ((!vtx.srcp() && !vtx.defaultp()) || !vtx.hasSinks()) {
                সূচক(vtx) = 0;
                component(vtx) = 0;
                চালু;
            }
            সূচক(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Initialize state of operation vertices
        জন্য (ধ্রুবক DfgVertex& vtx : m_dfg.opVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            যদি (!vtx.nInputs() || !vtx.hasSinks()) {
                সূচক(vtx) = 0;
                component(vtx) = 0;
                চালু;
            }
            সূচক(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Start traversals through not yet visited variables
        জন্য (ধ্রুবক DfgVertexVar& vtx : m_dfg.varVertices()) {
            যদি (সূচক(vtx) == UNASSIGNED) visitColorSCCs(vtx);
        }

        // Start traversals through not yet visited operations
        জন্য (ধ্রুবক DfgVertex& vtx : m_dfg.opVertices()) {
            যদি (সূচক(vtx) == UNASSIGNED) visitColorSCCs(vtx);
        }
    }

    explicit ColorStronglyConnectedComponents(ধ্রুবক DfgGraph& dfg, DfgUserMap<uint64_t>& map)
        : m_dfg{dfg}
        , m_map{map} {
        UASSERT(dfg.আকার_2() < UNASSIGNED, "Graph too big " << dfg.name());
        // Yet another implementation of Pearce's algorithm.
        colorSCCs();
        // Re-assign mapped values
        m_dfg.forEachVertex([&](ধ্রুবক DfgVertex& vtx) {
            ধ্রুবক uint64_t c = component(vtx);
            map[vtx] = c;
        });
    }

সর্বজনীন:
    // See declaration of  V3DfgPasses::colorStronglyConnectedComponents
    স্থির uint32_t apply(ধ্রুবক DfgGraph& dfg, DfgUserMap<uint64_t>& map) {
        ফেরত ColorStronglyConnectedComponents{dfg, map}.m_nonTrivialSCCs;
    }
};

uint32_t V3DfgPasses::colorStronglyConnectedComponents(ধ্রুবক DfgGraph& dfg,
                                                       DfgUserMap<uint64_t>& map) {
    ফেরত ColorStronglyConnectedComponents::apply(dfg, map);
}
