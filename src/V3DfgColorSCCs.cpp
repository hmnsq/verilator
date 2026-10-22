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

#అన్తర్భావ "V3Dfg.h"
#అన్తర్భావ "V3DfgPasses.h"

#అన్తర్భావ <limits>
#అన్తర్భావ <vector>

తరగతి ColorStronglyConnectedComponents final {
    static_assert(పరిమాణం(uint32_t[2]) == పరిమాణం(uint64_t), "Incorrect overlay size");

    // CONSTANTS
    స్థిర constexpr uint32_t UNASSIGNED = std::numeric_limits<uint32_t>::max();

    // STATE
    స్థిరరాశి Dfg_లేఖాచిత్రం& m_dfg;  // The input graph
    DfgUserMap<uint64_t>& m_map;  // The result map we are computing - also used for traversal
    uint32_t m_nonTrivialSCCs = 0;  // Number of non-trivial SCCs in the graph
    uint32_t m_సూచీ = 0;  // Visitation index counter
    std::సదిశ<స్థిరరాశి Dfg_శీర్షం*> m_దొంతర;  // The stack used by the algorithm

    // METHODS
    // Use the bottom 32-bit word as the component number
    uint32_t& component(స్థిరరాశి Dfg_శీర్షం& vtx) {  //
        తిరిగి reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[0];
    }
    // Use the top 32-bit word as the visitation index
    uint32_t& సూచీ(స్థిరరాశి Dfg_శీర్షం& vtx) {  //
        తిరిగి reinterpret_cast<uint32_t (&)[2]>(m_map[vtx])[1];
    }

    శూన్య సందర్శించు_Color_SCCs(స్థిరరాశి Dfg_శీర్షం& vtx) {
        UDEBUGONLY(UASSERT_OBJ(సూచీ(vtx) == UNASSIGNED, &vtx, "Already visited vertex"););

        // Visiting vertex
        స్థిరరాశి కొలత_t మూలం_సూచీ = సూచీ(vtx) = ++m_సూచీ;

        // Visit children
        vtx.foreachSink([&](స్థిరరాశి Dfg_శీర్షం& బిడ్డ) {
            // If the child has not yet been visited, then continue traversal
            ఒకవేళ (సూచీ(బిడ్డ) == UNASSIGNED) సందర్శించు_Color_SCCs(బిడ్డ);
            // If the child is not in an SCC
            ఒకవేళ (component(బిడ్డ) == UNASSIGNED) {
                ఒకవేళ (సూచీ(vtx) > సూచీ(బిడ్డ)) సూచీ(vtx) = సూచీ(బిడ్డ);
            }
            తిరిగి అబద్ధం;
        });

        ఒకవేళ (సూచీ(vtx) == మూలం_సూచీ) {
            // This is the 'root' of an SCC

            // A trivial SCC contains only a single vertex
            స్థిరరాశి తార్కిక isTrivial = m_దొంతర.ఖాళీలు() || సూచీ(*m_దొంతర.వెనుక()) < మూలం_సూచీ;
            // We also need a separate component for vertices that drive themselves (which can
            // happen for input like 'assign a = a'), as we want to extract them (they are cyclic).
            స్థిరరాశి తార్కిక drivesSelf = vtx.foreachSink([&](స్థిరరాశి Dfg_శీర్షం& sink) {  //
                తిరిగి &vtx == &sink;
            });

            ఒకవేళ (!isTrivial || drivesSelf) {
                // Allocate new component
                ++m_nonTrivialSCCs;
                component(vtx) = m_nonTrivialSCCs;
                అయితే (!m_దొంతర.ఖాళీలు()) {
                    // Only higher nodes belong to the same SCC
                    ఒకవేళ (సూచీ(*m_దొంతర.వెనుక()) < మూలం_సూచీ) ఆపు;
                    component(*m_దొంతర.వెనుక()) = m_nonTrivialSCCs;
                    m_దొంతర.తీయి_వెనుక();
                }
            } లేకపోతే {
                // Trivial SCC (and does not drive itself), so acyclic. Keep it in original graph.
                component(vtx) = 0;
            }
        } లేకపోతే {
            // Not the root of an SCC
            m_దొంతర.నెట్టు_వెనుక(&vtx);
        }
    }

    శూన్య colorSCCs() {
        // We know constant nodes have no input edges, so they cannot be part
        // of a non-trivial SCC. Mark them as such without any real traversals.
        కొరకు (స్థిరరాశి DfgConst& vtx : m_dfg.constVertices()) {
            సూచీ(vtx) = 0;
            component(vtx) = 0;
        }

        // We know ast references have no output or input edges
        కొరకు (స్థిరరాశి Dfg_శీర్షం_Ast& vtx : m_dfg.astVertices()) {
            సూచీ(vtx) = 0;
            component(vtx) = 0;
        }

        // Initialize state of variable vertices
        కొరకు (స్థిరరాశి Dfg_శీర్షం_Var& vtx : m_dfg.varVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            ఒకవేళ ((!vtx.srcp() && !vtx.defaultp()) || !vtx.hasSinks()) {
                సూచీ(vtx) = 0;
                component(vtx) = 0;
                కొనసాగు;
            }
            సూచీ(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Initialize state of operation vertices
        కొరకు (స్థిరరాశి Dfg_శీర్షం& vtx : m_dfg.opVertices()) {
            // If it has no inputs or no outputs, it cannot be part of a non-trivial SCC.
            ఒకవేళ (!vtx.nInputs() || !vtx.hasSinks()) {
                సూచీ(vtx) = 0;
                component(vtx) = 0;
                కొనసాగు;
            }
            సూచీ(vtx) = UNASSIGNED;
            component(vtx) = UNASSIGNED;
        }

        // Start traversals through not yet visited variables
        కొరకు (స్థిరరాశి Dfg_శీర్షం_Var& vtx : m_dfg.varVertices()) {
            ఒకవేళ (సూచీ(vtx) == UNASSIGNED) సందర్శించు_Color_SCCs(vtx);
        }

        // Start traversals through not yet visited operations
        కొరకు (స్థిరరాశి Dfg_శీర్షం& vtx : m_dfg.opVertices()) {
            ఒకవేళ (సూచీ(vtx) == UNASSIGNED) సందర్శించు_Color_SCCs(vtx);
        }
    }

    స్పష్టమైన ColorStronglyConnectedComponents(స్థిరరాశి Dfg_లేఖాచిత్రం& dfg, DfgUserMap<uint64_t>& map)
        : m_dfg{dfg}
        , m_map{map} {
        UASSERT(dfg.కొలత() < UNASSIGNED, "Graph too big " << dfg.name());
        // Yet another implementation of Pearce's algorithm.
        colorSCCs();
        // Re-assign mapped values
        m_dfg.for_Each_శీర్షం([&](స్థిరరాశి Dfg_శీర్షం& vtx) {
            స్థిరరాశి uint64_t c = component(vtx);
            map[vtx] = c;
        });
    }

బహిరంగ:
    // See declaration of  V3DfgPasses::colorStronglyConnectedComponents
    స్థిర uint32_t apply(స్థిరరాశి Dfg_లేఖాచిత్రం& dfg, DfgUserMap<uint64_t>& map) {
        తిరిగి ColorStronglyConnectedComponents{dfg, map}.m_nonTrivialSCCs;
    }
};

uint32_t V3DfgPasses::colorStronglyConnectedComponents(స్థిరరాశి Dfg_లేఖాచిత్రం& dfg,
                                                       DfgUserMap<uint64_t>& map) {
    తిరిగి ColorStronglyConnectedComponents::apply(dfg, map);
}
