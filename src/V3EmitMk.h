// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit Makefile
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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3EMITMK_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3EMITMK_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

class V3_Hier_ನಕ್ಷೆ;

//============================================================================

class V3EmitMk final {
ಸಾರ್ವಜನಿಕ:
    // Number of source files after which to use parallel compiles
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t PARALLEL_FILE_CNT_ಮಿತಿ = 128;

    ಸ್ಥಿರ ಶೂನ್ಯ emitmk() VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ emitHierVerilation(ಸ್ಥಿರಾಂಕ V3_Hier_ನಕ್ಷೆ* planp) VL_MT_DISABLED;
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
