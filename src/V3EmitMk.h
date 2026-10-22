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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3EMITMK_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3EMITMK_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

class V3_Hier_ਗ੍ਰਾਫ;

//============================================================================

class V3EmitMk final {
ਜਨਤਕ:
    // Number of source files after which to use parallel compiles
    ਸਥਿਰ ਅਚਲ ਮਾਪ_t PARALLEL_FILE_CNT_ਹੱਦ = 128;

    ਸਥਿਰ ਖਾਲੀ emitmk() VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ emitHierVerilation(ਅਚਲ V3_Hier_ਗ੍ਰਾਫ* planp) VL_MT_DISABLED;
};

#ਜੇ_ਅੰਤ  // Guard
