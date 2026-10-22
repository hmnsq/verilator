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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

class AstClass;
class AstFunc;
class AstNetlist;
class Ast_കണ്ണി_Module;

class VMemberMap;

class V3Randomize final {
പൊതു:
    സ്ഥിതം ശൂന്യം randomizeNetlist(AstNetlist* nodep) VL_MT_DISABLED;

    സ്ഥിതം AstFunc* newRandomizeFunc(VMemberMap& memberMap, AstClass* nodep,
                                     സ്ഥിരം std::string& name = "randomize",
                                     bool allowVirtual = true,
                                     bool childDType = false) VL_MT_DISABLED;
    സ്ഥിതം AstFunc* newRandomizeStdFunc(Ast_കണ്ണി_Module* nodep,
                                        സ്ഥിരം std::string& name) VL_MT_DISABLED;
    സ്ഥിതം AstFunc* newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) VL_MT_DISABLED;
};

#endif  // Guard
