// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Prepare AST for features features
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

#నిర్వచితం_కాకపోతే _V3TIMING_H_
#నిర్వచించు _V3TIMING_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"

//============================================================================

class V3Timing final {
బహిరంగ:
    స్థిర శూన్య timingAll(AstNetlist* nodep) VL_MT_DISABLED;
    // Scale a delay in the given time unit to time precision ticks, rounding real delays; #1step
    // is one tick
    స్థిర Ast_కణుపు_సమాసం* delay_గమ్యం_Ticks(Ast_కణుపు_సమాసం* valuep, VTimescale timeunit) VL_MT_DISABLED;
};

#అయితే_ముగింపు  // Guard
