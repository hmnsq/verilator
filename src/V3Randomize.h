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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3_RANDOMIZE_ವಿಧಾನ_H
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3_RANDOMIZE_ವಿಧಾನ_H

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

class AstClass;
class AstFunc;
class AstNetlist;
class Ast_ಗಂಟು_Module;

class VMemberMap;

class V3Randomize final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ randomizeNetlist(AstNetlist* nodep) VL_MT_DISABLED;

    ಸ್ಥಿರ AstFunc* newRandomizeFunc(VMemberMap& memberMap, AstClass* nodep,
                                     ಸ್ಥಿರಾಂಕ std::string& name = "randomize",
                                     bool allowVirtual = true,
                                     bool ಮಗು_DType = false) VL_MT_DISABLED;
    ಸ್ಥಿರ AstFunc* newRandomizeStdFunc(Ast_ಗಂಟು_Module* nodep,
                                        ಸ್ಥಿರಾಂಕ std::string& name) VL_MT_DISABLED;
    ಸ್ಥಿರ AstFunc* newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) VL_MT_DISABLED;
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
