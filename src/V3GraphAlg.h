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

#اگر_غیر_تعریف_شدہ VERILATOR_V3GRAPHALG_H_
#تعریف_کرو VERILATOR_V3GRAPHALG_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Global.h"
#شامل "V3Graph.h"

//=============================================================================
// Algorithms - common class
// For internal use, most graph algorithms use this as a base class

سانچہ <typename T_گراف = V3_گراف>  // Or sometimes const V3Graph
class گراف_Alg VL_NOT_FINAL {
محفوظ:
    T_گراف* ثابت m_graphp;  // Graph we're operating upon
    ثابت V3_کنارہ_Func_P m_کنارہ_Funcp;  // Function that says we follow this edge
    // CONSTRUCTORS
    گراف_Alg(T_گراف* graphp, V3_کنارہ_Func_P کنارہ_Funcp)
        : m_graphp{graphp}
        , m_کنارہ_Funcp{کنارہ_Funcp} {}
    ~گراف_Alg() = طےشدہ;
    // METHODS
    bool follow_کنارہ(V3_گراف_کنارہ* edgep) { واپس (edgep->وزن_2() && (m_کنارہ_Funcp)(edgep)); }
};

//============================================================================

#اگر_ختم  // Guard
