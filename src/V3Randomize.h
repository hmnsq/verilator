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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3_RANDOMIZE_ਵਿਧੀ_H
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3_RANDOMIZE_ਵਿਧੀ_H

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

class AstClass;
class AstFunc;
class AstNetlist;
class Ast_ਗੰਢ_Module;

class VMemberMap;

class V3Randomize final {
ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ randomizeNetlist(AstNetlist* nodep) VL_MT_DISABLED;

    ਸਥਿਰ AstFunc* newRandomizeFunc(VMemberMap& memberMap, AstClass* nodep,
                                     ਅਚਲ std::string& name = "randomize",
                                     bool allowVirtual = true,
                                     bool ਬੱਚਾ_DType = false) VL_MT_DISABLED;
    ਸਥਿਰ AstFunc* newRandomizeStdFunc(Ast_ਗੰਢ_Module* nodep,
                                        ਅਚਲ std::string& name) VL_MT_DISABLED;
    ਸਥਿਰ AstFunc* newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) VL_MT_DISABLED;
};

#ਜੇ_ਅੰਤ  // Guard
