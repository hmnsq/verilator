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

#که_نه_تعریف_شوی VERILATOR_V3GRAPHALG_H_
#تعریف_کړه VERILATOR_V3GRAPHALG_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Global.h"
#ضمن "V3Graph.h"

//=============================================================================
// Algorithms - common class
// For internal use, most graph algorithms use this as a base class

کينډۍ <typename T_ګراف = V3_ګراف>  // Or sometimes const V3Graph
class ګراف_Alg VL_NOT_FINAL {
محمي:
    T_ګراف* ثابت m_graphp;  // Graph we're operating upon
    ثابت V3_څنډه_Func_P m_څنډه_Funcp;  // Function that says we follow this edge
    // CONSTRUCTORS
    ګراف_Alg(T_ګراف* graphp, V3_څنډه_Func_P څنډه_Funcp)
        : m_graphp{graphp}
        , m_څنډه_Funcp{څنډه_Funcp} {}
    ~ګراف_Alg() = افتراضي;
    // METHODS
    bool follow_څنډه(V3_ګراف_څنډه* edgep) { أرجع (edgep->وزن_2() && (m_څنډه_Funcp)(edgep)); }
};

//============================================================================

#د_که_پای  // Guard
