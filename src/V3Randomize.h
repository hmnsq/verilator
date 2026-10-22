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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

class AstClass;
class AstFunc;
class AstNetlist;
class Ast_ગાંઠ_Module;

class VMemberMap;

class V3Randomize final {
જાહેર:
    સ્થિર શૂન્ય randomizeNetlist(AstNetlist* nodep) VL_MT_DISABLED;

    સ્થિર AstFunc* newRandomizeFunc(VMemberMap& memberMap, AstClass* nodep,
                                     અચલ std::string& name = "randomize",
                                     bool allowVirtual = true,
                                     bool childDType = false) VL_MT_DISABLED;
    સ્થિર AstFunc* newRandomizeStdFunc(Ast_ગાંઠ_Module* nodep,
                                        અચલ std::string& name) VL_MT_DISABLED;
    સ્થિર AstFunc* newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) VL_MT_DISABLED;
};

#endif  // Guard
