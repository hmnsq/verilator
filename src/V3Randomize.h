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

#अगर_अघोषित VERILATOR_V3RANDOMIZE_METHOD_H_
#घोषणा VERILATOR_V3RANDOMIZE_METHOD_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

class AstClass;
class AstFunc;
class AstNetlist;
class Ast_गाँठ_Module;

class VMemberMap;

class V3Randomize final {
खुला:
    अटल व्योम randomizeNetlist(AstNetlist* nodep) VL_MT_DISABLED;

    अटल AstFunc* newRandomizeFunc(VMemberMap& memberMap, AstClass* nodep,
                                     स्थिर std::string& name = "randomize",
                                     bool allowVirtual = true,
                                     bool childDType = false) VL_MT_DISABLED;
    अटल AstFunc* newRandomizeStdFunc(Ast_गाँठ_Module* nodep,
                                        स्थिर std::string& name) VL_MT_DISABLED;
    अटल AstFunc* newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) VL_MT_DISABLED;
};

#पूर्ण_अगर  // Guard
