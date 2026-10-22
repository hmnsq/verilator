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

#شامل "V3Dfg.h"
#شامل "V3DfgPasses.h"

#شامل <limits>
#شامل <vector>

جماعت ColorStronglyConnectedComponents final {
    static_assert(حجم(uint32_t[2]) == حجم(uint64_t), "Incorrect overlay size");

    // CONSTANTS
    ساکن constexpr uint32_t UNASSIGNED = std::numeric_limits<uint32_t>::max();

    // STATE
    ثابت Dfg_گراف& m_dfg;  // The input graph
    DfgUserMap<uint64_t>& m_map;  // The result map we are computing - also used for traversal
    uint32_t m_nonTrivialSCCs = 0;  // Number of non-trivial SCCs in the graph
    uint32_t m_اشاریہ = 0;  // Visitation index counter
    std::سمتیہ<ثابت Dfg_راس*> m_ڈھیر;  // The stack used by the algorithm

    // METHODS
    // Use the bottom 32-bit word as the component number
    uint32_t& component(ثابت Dfg_راس& vtx) {  //
        واپس reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[0];
    }
    // Use the top 32-bit word as the visitation index
    uint32_t& اشاریہ(ثابت Dfg_راس& vtx) {  //
        واپس reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[1];
    }

    خالی دورہ_Color_SCCs(ثابت Dfg_راس& vtx) {
        UDEBUGONLY(UASSERT_OBJ(اشاریہ(vtx) == UNASSIGNED, &vtx, "Already visited vertex"););

        // Visiting vertex
        ثابت سائز_t جڑ_اشاریہ = اشاریہ(vtx) = ++m_اشاریہ;

        // Visit children
        vtx.foreachSink([&](ثابت Dfg_راس& بچہ) {
            // If the child has not yet been visited, then continue traversal
            اگر (اشاریہ(بچہ) == UNASSIGNED) دورہ_Color_SCCs(بچہ);
            // If the child is not in an SCC
            اگر (component(بچہ) == UNASSIGNED) {
                اگر (اشاریہ(vtx) > اشاریہ(بچہ)) اشاریہ(vtx) = اشاریہ(بچہ);
            }
            واپس جھوٹ;
        });

        اگر (اشاریہ(vtx) == جڑ_اشاریہ) {
            // This is the 'root' of an SCC

            // A trivial SCC contains only a single vertex
            ثابت منطقی isTrivial = m_ڈھیر.خالی_خانے() || اشاریہ(*m_ڈھیر.عقب()) < جڑ_اشاریہ;
            // We also need a separate component for vertices that drive themselves (which can
            // happen for input like 'assign a = a'), as we want to extract them (they are cyclic).
            ثابت منطقی drivesSelf = vtx.foreachSink([&](ثابت Dfg_راس& sink) {  //
                واپس &vtx == &sink;
            });

            اگر (!isTrivial || drivesSelf) {
                // Allocate new component
                ++m_nonTrivialSCCs;
                component(vtx) = m_nonTrivialSCCs;
                جب (!m_ڈھیر.خالی_خانے()) {
                    // Only higher nodes belong to the same SCC
                    اگر (اشاریہ(*m_ڈھیر.عقب()) < جڑ_اشاریہ) توڑو;
                    component(*m_ڈھیر.عقب()) = m_nonTrivialSCCs;
                    m_ڈھیر.نکالو_عقب();
                }
            } ورنہ {
                // Trivial SCC (and does not drive itself), so acyclic. Keep it in original graph.
                component(vtx) = 0;
            }
        } ورنہ {
            // Not the root of an SCC
            m_ڈھیر.ڈالو_عقب(&vtx);
        }
    }

    خالی colorSCCs() {
        // We know constant nodes have no input edges, so they cannot be part
        // of a non-trivial SCC. Mark them as such without any real traversals.
        برائے (ثابت DfgConst& vtx : m_dfg.constVertices()) {
            اشاریہ(vtx) = 0;
            component(vtx) = 0;
        }

        // We know ast references have no output or input edges
        برائے (ثابت Dfg_راس_Ast& vtx : m_dfg.astVertices()) {
            اشاریہ(vtx) = 0;
            component(vtx) = 0;
        }

        // Initialize state of variable vertices
        برائے (ثابت Dfg_راس_Var& vtx : m_dfg.varVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            اگر ((!vtx.srcp() && !vtx.defaultp()) || !vtx.hasSinks()) {
                اشاریہ(vtx) = 0;
                component(vtx) = 0;
                جاری;
            }
            اشاریہ(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Initialize state of operation vertices
        برائے (ثابت Dfg_راس& vtx : m_dfg.opVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            اگر (!vtx.nInputs() || !vtx.hasSinks()) {
                اشاریہ(vtx) = 0;
                component(vtx) = 0;
                جاری;
            }
            اشاریہ(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Start traversals through not yet visited variables
        برائے (ثابت Dfg_راس_Var& vtx : m_dfg.varVertices()) {
            اگر (اشاریہ(vtx) == UNASSIGNED) دورہ_Color_SCCs(vtx);
        }

        // Start traversals through not yet visited operations
        برائے (ثابت Dfg_راس& vtx : m_dfg.opVertices()) {
            اگر (اشاریہ(vtx) == UNASSIGNED) دورہ_Color_SCCs(vtx);
        }
    }

    صریح ColorStronglyConnectedComponents(ثابت Dfg_گراف& dfg, DfgUserMap<uint64_t>& map)
        : m_dfg{dfg}
        , m_map{map} {
        UASSERT(dfg.سائز() < UNASSIGNED, "Graph too big " << dfg.name());
        // Yet another implementation of Pearce's algorithm.
        colorSCCs();
        // Re-assign mapped values
        m_dfg.for_Each_راس([&](ثابت Dfg_راس& vtx) {
            ثابت uint64_t c = component(vtx);
            map[vtx] = c;
        });
    }

عوامی:
    // See declaration of  V3DfgPasses::colorStronglyConnectedComponents
    ساکن uint32_t apply(ثابت Dfg_گراف& dfg, DfgUserMap<uint64_t>& map) {
        واپس ColorStronglyConnectedComponents{dfg, map}.m_nonTrivialSCCs;
    }
};

uint32_t V3DfgPasses::colorStronglyConnectedComponents(ثابت Dfg_گراف& dfg,
                                                       DfgUserMap<uint64_t>& map) {
    واپس ColorStronglyConnectedComponents::apply(dfg, map);
}
