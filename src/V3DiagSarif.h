// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Diag Sarif output file
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

#నిర్వచితం_కాకపోతే VERILATOR_V3DIAGSARIF_H_
#నిర్వచించు VERILATOR_V3DIAGSARIF_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ <V3Error.h>

//============================================================================

class V3DiagSarif final {
బహిరంగ:
    స్థిర శూన్య నెట్టు_సందేశం(స్థిరరాశి VError_సందేశం& సందేశం_2) VL_MT_DISABLED;
    స్థిర శూన్య output(bool success) VL_MT_DISABLED;
};

#అయితే_ముగింపు  // Guard
