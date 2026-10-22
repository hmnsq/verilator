// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Add common contents to modules
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

#నిర్వచితం_కాకపోతే VERILATOR_V3COMMON_H_
#నిర్వచించు VERILATOR_V3COMMON_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

class Ast_కణుపు_DType;

//============================================================================

class V3Common final {
బహిరంగ:
    స్థిర శూన్య commonAll() VL_MT_DISABLED;
    స్థిర string make_గమ్యం_String_Call(Ast_కణుపు_DType* nodep, స్థిరరాశి std::string& lhs);
};

#అయితే_ముగింపు  // Guard
