// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Dead branch elimination
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

#ifndef VERILATOR_V3DEAD_H_
#define VERILATOR_V3DEAD_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

class AstNetlist;

//============================================================================

class V3Dead final {
عمومی:
    // Modules, no vars/dtypes
    ایستا تهی deadifyModules(AstNetlist* nodep) VL_MT_DISABLED;
    // Modules, Data types
    ایستا تهی deadifyDTypes(AstNetlist* nodep) VL_MT_DISABLED;
    ایستا تهی deadifyDTypesScoped(AstNetlist* nodep) VL_MT_DISABLED;
    // Everything that's possible
    ایستا تهی deadifyAll(AstNetlist* nodep) VL_MT_DISABLED;
    ایستا تهی deadifyAllScoped(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
