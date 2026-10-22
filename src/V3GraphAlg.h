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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3Graph.h"

//=============================================================================
// Algorithms - common class
// For internal use, most graph algorithms use this as a base class

template <typename T_Graph = V3Graph>  // Or sometimes const V3Graph
class GraphAlg VL_NOT_FINAL {
பாதுகாக்கப்பட்ட:
    T_Graph* மாறிலி m_graphp;  // Graph we're operating upon
    மாறிலி V3EdgeFuncP m_edgeFuncp;  // Function that says we follow this edge
    // CONSTRUCTORS
    GraphAlg(T_Graph* graphp, V3EdgeFuncP edgeFuncp)
        : m_graphp{graphp}
        , m_edgeFuncp{edgeFuncp} {}
    ~GraphAlg() = இயல்பு;
    // METHODS
    bool followEdge(V3GraphEdge* edgep) { திரும்பு (edgep->weight() && (m_edgeFuncp)(edgep)); }
};

//============================================================================

#endif  // Guard
