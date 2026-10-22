// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Find broken links in tree
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

#నిర్వచితం_కాకపోతే VERILATOR_V3BROKEN_H_
#నిర్వచించు VERILATOR_V3BROKEN_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

//============================================================================

// Forward declare so we can include this in V3Ast.h
class AstNetlist;
class Ast_కణుపు;

class V3Broken final {
బహిరంగ:
    స్థిర శూన్య brokenAll(AstNetlist* nodep);
    స్థిర bool isLinkable(స్థిరరాశి Ast_కణుపు* nodep);
    స్థిర శూన్య కూడు_Newed(స్థిరరాశి Ast_కణుపు* nodep);
    స్థిర శూన్య deleted(స్థిరరాశి Ast_కణుపు* nodep);
    // Called on error to say may be inside visitor,
    // Disables checks that may misfire if not called at stable point between visitors
    స్థిర శూన్య allowMidvisitorCheck(bool flag);
    స్థిర శూన్య selfTest();
};

#అయితే_ముగింపు  // Guard
