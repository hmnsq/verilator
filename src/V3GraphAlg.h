// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Graph algorithm base class
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

#ifndef VERILATOR_V3GRAPHALG_H_
#define VERILATOR_V3GRAPHALG_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"

//=============================================================================
// Algorithms - common class
// For internal use, most graph algorithms use this as a base class

template <typename T_Graph = V3Graph>  // Or sometimes const V3Graph
class GraphAlg VL_NOT_FINAL {
ರಕ್ಷಿತ:
    T_Graph* ಧ್ರುವ m_graphp;  // Graph we're operating upon
    ಧ್ರುವ V3EdgeFuncP m_edgeFuncp;  // Function that says we follow this edge
    // CONSTRUCTORS
    GraphAlg(T_Graph* graphp, V3EdgeFuncP edgeFuncp)
        : m_graphp{graphp}
        , m_edgeFuncp{edgeFuncp} {}
    ~GraphAlg() = ಸ್ವಭಾವ;
    // METHODS
    bool followEdge(V3GraphEdge* edgep) { ಪ್ರತ್ಯಾಗಮ (edgep->weight() && (m_edgeFuncp)(edgep)); }
};

//============================================================================

#endif  // Guard
