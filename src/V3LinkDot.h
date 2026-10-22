// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Link XREF signals/functions together
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

#నిర్వచితం_కాకపోతే VERILATOR_V3LINKDOT_H_
#నిర్వచించు VERILATOR_V3LINKDOT_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Error.h"

//============================================================================

గణన VLink_Dot_అడుగు : uint8_t { LDS_PRIMARY, LDS_PARAMED, LDS_SCOPED };

class V3LinkDot final {
    స్థిర శూన్య dumpSubstep(స్థిరరాశి string& name) VL_MT_DISABLED;
    స్థిర శూన్య linkDotGuts(AstNetlist* rootp, VLink_Dot_అడుగు అడుగు) VL_MT_DISABLED;

బహిరంగ:
    స్థిర శూన్య linkDotPrimary(AstNetlist* nodep) VL_MT_DISABLED;
    స్థిర శూన్య linkDotParamed(AstNetlist* nodep) VL_MT_DISABLED;
    స్థిర శూన్య linkDotScope(AstNetlist* nodep) VL_MT_DISABLED;
};

#అయితే_ముగింపు  // Guard
