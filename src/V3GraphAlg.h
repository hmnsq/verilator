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

#اگر_تعریف_نشده VERILATOR_V3GRAPHALG_H_
#تعریف_کن VERILATOR_V3GRAPHALG_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Global.h"
#ضمن "V3Graph.h"

//=============================================================================
// Algorithms - common class
// For internal use, most graph algorithms use this as a base class

قالب <typename T_گراف = V3_گراف>  // Or sometimes const V3Graph
class گراف_Alg VL_NOT_FINAL {
محمي:
    T_گراف* ثابت m_graphp;  // Graph we're operating upon
    ثابت V3_یال_Func_P m_یال_Funcp;  // Function that says we follow this edge
    // CONSTRUCTORS
    گراف_Alg(T_گراف* graphp, V3_یال_Func_P یال_Funcp)
        : m_graphp{graphp}
        , m_یال_Funcp{یال_Funcp} {}
    ~گراف_Alg() = افتراضي;
    // METHODS
    bool follow_یال(V3_گراف_یال* edgep) { أرجع (edgep->وزن_2() && (m_یال_Funcp)(edgep)); }
};

//============================================================================

#پایان_اگر  // Guard
