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

#اگر_غیر_تعریف_شدہ VERILATOR_V3_RANDOMIZE_طریقہ_H
#تعریف_کرو VERILATOR_V3_RANDOMIZE_طریقہ_H

#شامل "config_build.h"
#شامل "verilatedos.h"

class AstClass;
class AstFunc;
class AstNetlist;
class Ast_گرہ_Module;

class VMemberMap;

class V3Randomize final {
عوامی:
    ساکن خالی randomizeNetlist(AstNetlist* nodep) VL_MT_DISABLED;

    ساکن AstFunc* newRandomizeFunc(VMemberMap& memberMap, AstClass* nodep,
                                     ثابت std::string& name = "randomize",
                                     bool allowVirtual = true,
                                     bool بچہ_DType = false) VL_MT_DISABLED;
    ساکن AstFunc* newRandomizeStdFunc(Ast_گرہ_Module* nodep,
                                        ثابت std::string& name) VL_MT_DISABLED;
    ساکن AstFunc* newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) VL_MT_DISABLED;
};

#اگر_ختم  // Guard
