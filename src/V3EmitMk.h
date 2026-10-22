// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit Makefile
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

#నిర్వచితం_కాకపోతే VERILATOR_V3EMITMK_H_
#నిర్వచించు VERILATOR_V3EMITMK_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

class V3_Hier_లేఖాచిత్రం;

//============================================================================

class V3EmitMk final {
బహిరంగ:
    // Number of source files after which to use parallel compiles
    స్థిర స్థిరరాశి కొలత_t PARALLEL_FILE_CNT_పరిమితి = 128;

    స్థిర శూన్య emitmk() VL_MT_DISABLED;
    స్థిర శూన్య emitHierVerilation(స్థిరరాశి V3_Hier_లేఖాచిత్రం* planp) VL_MT_DISABLED;
};

#అయితే_ముగింపు  // Guard
