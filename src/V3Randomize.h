// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Generate randomization procedures
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

#ifndef VERILATOR_V3RANDOMIZE_METHOD_H_
#define VERILATOR_V3RANDOMIZE_METHOD_H_

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

class AstClass;
class AstFunc;
class AstNetlist;
class Ast_முனை_Module;

class VMemberMap;

class V3Randomize final {
பொது:
    நிலையான வெறுமை randomizeNetlist(AstNetlist* nodep) VL_MT_DISABLED;

    நிலையான AstFunc* newRandomizeFunc(VMemberMap& memberMap, AstClass* nodep,
                                     மாறிலி std::string& name = "randomize",
                                     bool allowVirtual = true,
                                     bool childDType = false) VL_MT_DISABLED;
    நிலையான AstFunc* newRandomizeStdFunc(Ast_முனை_Module* nodep,
                                        மாறிலி std::string& name) VL_MT_DISABLED;
    நிலையான AstFunc* newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) VL_MT_DISABLED;
};

#endif  // Guard
