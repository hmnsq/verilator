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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3GRAPHALG_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3GRAPHALG_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"

//=============================================================================
// Algorithms - common class
// For internal use, most graph algorithms use this as a base class

ਸਾਂਚਾ <typename T_ਗ੍ਰਾਫ = V3_ਗ੍ਰਾਫ>  // Or sometimes const V3Graph
class ਗ੍ਰਾਫ_Alg VL_NOT_FINAL {
ਸੁਰੱਖਿਅਤ:
    T_ਗ੍ਰਾਫ* ਅਚਲ m_graphp;  // Graph we're operating upon
    ਅਚਲ V3_ਕਿਨਾਰਾ_Func_P m_ਕਿਨਾਰਾ_Funcp;  // Function that says we follow this edge
    // CONSTRUCTORS
    ਗ੍ਰਾਫ_Alg(T_ਗ੍ਰਾਫ* graphp, V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp)
        : m_graphp{graphp}
        , m_ਕਿਨਾਰਾ_Funcp{ਕਿਨਾਰਾ_Funcp} {}
    ~ਗ੍ਰਾਫ_Alg() = ਮੂਲ;
    // METHODS
    bool follow_ਕਿਨਾਰਾ(V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* edgep) { ਵਾਪਸ (edgep->ਭਾਰ_2() && (m_ਕਿਨਾਰਾ_Funcp)(edgep)); }
};

//============================================================================

#ਜੇ_ਅੰਤ  // Guard
