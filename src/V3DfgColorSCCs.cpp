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

صنف ColorStronglyConnectedComponents final {
    static_assert(حجم(uint32_t[2]) == حجم(uint64_t), "Incorrect overlay size");

    // CONSTANTS
    ساكن constexpr uint32_t UNASSIGNED = std::numeric_limits<uint32_t>::max();

    // STATE
    ثابت Dfg_گراف& m_dfg;  // The input graph
    DfgUserMap<uint64_t>& m_map;  // The result map we are computing - also used for traversal
    uint32_t m_nonTrivialSCCs = 0;  // Number of non-trivial SCCs in the graph
    uint32_t m_اشاریہ = 0;  // Visitation index counter
    std::سمتیہ<ثابت Dfg_راس*> m_ڈھیر;  // The stack used by the algorithm

    // METHODS
    // Use the bottom 32-bit word as the component number
    uint32_t& component(ثابت Dfg_راس& vtx) {  //
        أرجع reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[0];
    }
    // Use the top 32-bit word as the visitation index
    uint32_t& اشاریہ(ثابت Dfg_راس& vtx) {  //
        أرجع reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[1];
    }

    فراغ ویکھو_Color_SCCs(ثابت Dfg_راس& vtx) {
        UDEBUGONLY(UASSERT_OBJ(اشاریہ(vtx) == UNASSIGNED, &vtx, "Already visited vertex"););

        // Visiting vertex
        ثابت ناپ_t جڑ_اشاریہ = اشاریہ(vtx) = ++m_اشاریہ;

        // Visit children
        vtx.foreachSink([&](ثابت Dfg_راس& بچہ) {
            // If the child has not yet been visited, then continue traversal
            إذا (اشاریہ(بچہ) == UNASSIGNED) ویکھو_Color_SCCs(بچہ);
            // If the child is not in an SCC
            إذا (component(بچہ) == UNASSIGNED) {
                إذا (اشاریہ(vtx) > اشاریہ(بچہ)) اشاریہ(vtx) = اشاریہ(بچہ);
            }
            أرجع خطأ;
        });

        إذا (اشاریہ(vtx) == جڑ_اشاریہ) {
            // This is the 'root' of an SCC

            // A trivial SCC contains only a single vertex
            ثابت منطقي isTrivial = m_ڈھیر.خالی_تھاواں() || اشاریہ(*m_ڈھیر.پٹھ()) < جڑ_اشاریہ;
            // We also need a separate component for vertices that drive themselves (which can
            // happen for input like 'assign a = a'), as we want to extract them (they are cyclic).
            ثابت منطقي drivesSelf = vtx.foreachSink([&](ثابت Dfg_راس& sink) {  //
                أرجع &vtx == &sink;
            });

            إذا (!isTrivial || drivesSelf) {
                // Allocate new component
                ++m_nonTrivialSCCs;
                component(vtx) = m_nonTrivialSCCs;
                طالما (!m_ڈھیر.خالی_تھاواں()) {
                    // Only higher nodes belong to the same SCC
                    إذا (اشاریہ(*m_ڈھیر.پٹھ()) < جڑ_اشاریہ) اكسر;
                    component(*m_ڈھیر.پٹھ()) = m_nonTrivialSCCs;
                    m_ڈھیر.نکالو_پٹھ();
                }
            } وإلا {
                // Trivial SCC (and does not drive itself), so acyclic. Keep it in original graph.
                component(vtx) = 0;
            }
        } وإلا {
            // Not the root of an SCC
            m_ڈھیر.ڈالو_پٹھ(&vtx);
        }
    }

    فراغ colorSCCs() {
        // We know constant nodes have no input edges, so they cannot be part
        // of a non-trivial SCC. Mark them as such without any real traversals.
        لكل (ثابت DfgConst& vtx : m_dfg.constVertices()) {
            اشاریہ(vtx) = 0;
            component(vtx) = 0;
        }

        // We know ast references have no output or input edges
        لكل (ثابت Dfg_راس_Ast& vtx : m_dfg.astVertices()) {
            اشاریہ(vtx) = 0;
            component(vtx) = 0;
        }

        // Initialize state of variable vertices
        لكل (ثابت Dfg_راس_Var& vtx : m_dfg.varVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            إذا ((!vtx.srcp() && !vtx.defaultp()) || !vtx.hasSinks()) {
                اشاریہ(vtx) = 0;
                component(vtx) = 0;
                استمر;
            }
            اشاریہ(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Initialize state of operation vertices
        لكل (ثابت Dfg_راس& vtx : m_dfg.opVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            إذا (!vtx.nInputs() || !vtx.hasSinks()) {
                اشاریہ(vtx) = 0;
                component(vtx) = 0;
                استمر;
            }
            اشاریہ(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Start traversals through not yet visited variables
        لكل (ثابت Dfg_راس_Var& vtx : m_dfg.varVertices()) {
            إذا (اشاریہ(vtx) == UNASSIGNED) ویکھو_Color_SCCs(vtx);
        }

        // Start traversals through not yet visited operations
        لكل (ثابت Dfg_راس& vtx : m_dfg.opVertices()) {
            إذا (اشاریہ(vtx) == UNASSIGNED) ویکھو_Color_SCCs(vtx);
        }
    }

    واضح ColorStronglyConnectedComponents(ثابت Dfg_گراف& dfg, DfgUserMap<uint64_t>& map)
        : m_dfg{dfg}
        , m_map{map} {
        UASSERT(dfg.ناپ() < UNASSIGNED, "Graph too big " << dfg.name());
        // Yet another implementation of Pearce's algorithm.
        colorSCCs();
        // Re-assign mapped values
        m_dfg.for_Each_راس([&](ثابت Dfg_راس& vtx) {
            ثابت uint64_t c = component(vtx);
            map[vtx] = c;
        });
    }

عام:
    // See declaration of  V3DfgPasses::colorStronglyConnectedComponents
    ساكن uint32_t apply(ثابت Dfg_گراف& dfg, DfgUserMap<uint64_t>& map) {
        أرجع ColorStronglyConnectedComponents{dfg, map}.m_nonTrivialSCCs;
    }
};

uint32_t V3DfgPasses::colorStronglyConnectedComponents(ثابت Dfg_گراف& dfg,
                                                       DfgUserMap<uint64_t>& map) {
    أرجع ColorStronglyConnectedComponents::apply(dfg, map);
}
