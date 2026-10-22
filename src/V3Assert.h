// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Assertion expansion
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2005-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#నిర్వచితం_కాకపోతే VERILATOR_V3ASSERT_H_
#నిర్వచించు VERILATOR_V3ASSERT_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"

//============================================================================

class V3AssertCommon final {
బహిరంగ:
    స్థిర శూన్య collectDefaultDisable(AstNetlist* nodep) VL_MT_DISABLED;
    స్థిర శూన్య క్రింది_Sequence_Events(AstNetlist* nodep) VL_MT_DISABLED;
    స్థిర string assertCtlGetCall(స్థిరరాశి అక్షరం* query, VAssertType type,
                                   VAssertDirectiveType directiveType) VL_MT_DISABLED;
    // Check an SVA value is within the implementation limit, else report it on nodep
    స్థిర bool check_Sva_విలువ_హద్దు(Ast_కణుపు* nodep, స్థిరరాశి AstConst* constp,
                                   స్థిరరాశి అక్షరం* what) VL_MT_DISABLED;
    // Fold the bounds of a cycle delay in a sequence, and check they are non-negative
    // elaboration-time constants (IEEE 1800-2023 16.7). If not, report it on errp, and replace
    // them with constants, so later steps do not fail after the error.
    స్థిర శూన్య check_Sva_చక్రం_Delay(AstDelay* nodep, Ast_కణుపు* errp) VL_MT_DISABLED;
    // True if the expression references a formal argument of a sequence or property
    స్థిర bool refsSvaFormal(స్థిరరాశి Ast_కణుపు* nodep) VL_MT_DISABLED;
    // Check the cycle delays of a sequence or property instance, after its formal arguments are
    // replaced with the actual arguments (IEEE 1800-2023 16.8), reporting errors on instp
    స్థిర శూన్య checkSvaInstanceDelays(Ast_కణుపు* nodep, Ast_కణుపు* instp) VL_MT_DISABLED;
};

class V3Assert final {
బహిరంగ:
    స్థిర శూన్య assertAll(AstNetlist* nodep) VL_MT_DISABLED;
};

#అయితే_ముగింపు  // Guard
