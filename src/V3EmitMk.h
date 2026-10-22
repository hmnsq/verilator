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

#ifndef VERILATOR_V3EMITMK_H_
#define VERILATOR_V3EMITMK_H_

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

class V3HierGraph;

//============================================================================

class V3EmitMk final {
പൊതു:
    // Number of source files after which to use parallel compiles
    സ്ഥിതം സ്ഥിരം size_t PARALLEL_FILE_CNT_THRESHOLD = 128;

    സ്ഥിതം ശൂന്യം emitmk() VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം emitHierVerilation(സ്ഥിരം V3HierGraph* planp) VL_MT_DISABLED;
};

#endif  // Guard
