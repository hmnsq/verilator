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

#अगर_अघोषित VERILATOR_V3GRAPHALG_H_
#घोषणा VERILATOR_V3GRAPHALG_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Global.h"
#समावेश "V3Graph.h"

//=============================================================================
// Algorithms - common class
// For internal use, most graph algorithms use this as a base class

ढाँचा <typename T_Graph = V3Graph>  // Or sometimes const V3Graph
class GraphAlg VL_NOT_FINAL {
रक्षित:
    T_Graph* स्थिर m_graphp;  // Graph we're operating upon
    स्थिर V3EdgeFuncP m_edgeFuncp;  // Function that says we follow this edge
    // CONSTRUCTORS
    GraphAlg(T_Graph* graphp, V3EdgeFuncP edgeFuncp)
        : m_graphp{graphp}
        , m_edgeFuncp{edgeFuncp} {}
    ~GraphAlg() = शेष;
    // METHODS
    bool followEdge(V3GraphEdge* edgep) { वापस (edgep->weight() && (m_edgeFuncp)(edgep)); }
};

//============================================================================

#पूर्ण_अगर  // Guard
