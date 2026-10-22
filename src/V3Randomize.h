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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

class AstClass;
class AstFunc;
class AstNetlist;
class Ast_গ্রন্থি_Module;

class VMemberMap;

class V3Randomize final {
সর্বজনীন:
    স্থির শূন্য randomizeNetlist(AstNetlist* nodep) VL_MT_DISABLED;

    স্থির AstFunc* newRandomizeFunc(VMemberMap& memberMap, AstClass* nodep,
                                     ধ্রুবক std::string& name = "randomize",
                                     bool allowVirtual = true,
                                     bool childDType = false) VL_MT_DISABLED;
    স্থির AstFunc* newRandomizeStdFunc(Ast_গ্রন্থি_Module* nodep,
                                        ধ্রুবক std::string& name) VL_MT_DISABLED;
    স্থির AstFunc* newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) VL_MT_DISABLED;
};

#endif  // Guard
