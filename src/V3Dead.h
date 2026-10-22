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

#నిర్వచితం_కాకపోతే VERILATOR_V3DEAD_H_
#నిర్వచించు VERILATOR_V3DEAD_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

class AstNetlist;

//============================================================================

class V3Dead final {
బహిరంగ:
    // Modules, no vars/dtypes
    స్థిర శూన్య deadifyModules(AstNetlist* nodep) VL_MT_DISABLED;
    // Modules, Data types
    స్థిర శూన్య deadifyDTypes(AstNetlist* nodep) VL_MT_DISABLED;
    స్థిర శూన్య deadifyDTypesScoped(AstNetlist* nodep) VL_MT_DISABLED;
    // Everything that's possible
    స్థిర శూన్య deadifyAll(AstNetlist* nodep) VL_MT_DISABLED;
    స్థిర శూన్య deadifyAllScoped(AstNetlist* nodep) VL_MT_DISABLED;
};

#అయితే_ముగింపు  // Guard
