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

#నిర్వచితం_కాకపోతే VERILATOR_V3_RANDOMIZE_పద్ధతి_H
#నిర్వచించు VERILATOR_V3_RANDOMIZE_పద్ధతి_H

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

class AstClass;
class AstFunc;
class AstNetlist;
class Ast_కణుపు_Module;

class VMemberMap;

class V3Randomize final {
బహిరంగ:
    స్థిర శూన్య randomizeNetlist(AstNetlist* nodep) VL_MT_DISABLED;

    స్థిర AstFunc* newRandomizeFunc(VMemberMap& memberMap, AstClass* nodep,
                                     స్థిరరాశి std::string& name = "randomize",
                                     bool allowVirtual = true,
                                     bool బిడ్డ_DType = false) VL_MT_DISABLED;
    స్థిర AstFunc* newRandomizeStdFunc(Ast_కణుపు_Module* nodep,
                                        స్థిరరాశి std::string& name) VL_MT_DISABLED;
    స్థిర AstFunc* newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) VL_MT_DISABLED;
};

#అయితే_ముగింపు  // Guard
