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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

class AstNetlist;

//============================================================================

class V3Dead final {
പൊതു:
    // Modules, no vars/dtypes
    സ്ഥിതം ശൂന്യം deadifyModules(AstNetlist* nodep) VL_MT_DISABLED;
    // Modules, Data types
    സ്ഥിതം ശൂന്യം deadifyDTypes(AstNetlist* nodep) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം deadifyDTypesScoped(AstNetlist* nodep) VL_MT_DISABLED;
    // Everything that's possible
    സ്ഥിതം ശൂന്യം deadifyAll(AstNetlist* nodep) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം deadifyAllScoped(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
