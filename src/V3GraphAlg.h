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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3GRAPHALG_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3GRAPHALG_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"

//=============================================================================
// Algorithms - common class
// For internal use, most graph algorithms use this as a base class

ಮಾದರಿ <typename T_ನಕ್ಷೆ = V3_ನಕ್ಷೆ>  // Or sometimes const V3Graph
class ನಕ್ಷೆ_Alg VL_NOT_FINAL {
ಸಂರಕ್ಷಿತ:
    T_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ m_graphp;  // Graph we're operating upon
    ಸ್ಥಿರಾಂಕ V3_ಅಂಚು_Func_P m_ಅಂಚು_Funcp;  // Function that says we follow this edge
    // CONSTRUCTORS
    ನಕ್ಷೆ_Alg(T_ನಕ್ಷೆ* graphp, V3_ಅಂಚು_Func_P ಅಂಚು_Funcp)
        : m_graphp{graphp}
        , m_ಅಂಚು_Funcp{ಅಂಚು_Funcp} {}
    ~ನಕ್ಷೆ_Alg() = ಪೂರ್ವನಿಯೋಜಿತ;
    // METHODS
    bool follow_ಅಂಚು(V3_ನಕ್ಷೆ_ಅಂಚು* edgep) { ಹಿಂತಿರುಗಿ (edgep->ತೂಕ_2() && (m_ಅಂಚು_Funcp)(edgep)); }
};

//============================================================================

#ಆದರೆ_ಅಂತ್ಯ  // Guard
