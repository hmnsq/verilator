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

#ifndef VERILATOR_V3ASSERT_H_
#define VERILATOR_V3ASSERT_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"

//============================================================================

class V3AssertCommon final {
عمومی:
    ایستا تهی collectDefaultDisable(AstNetlist* nodep) VL_MT_DISABLED;
    ایستا تهی lowerSequenceEvents(AstNetlist* nodep) VL_MT_DISABLED;
    ایستا string assertCtlGetCall(ثابت نویسه* query, VAssertType type,
                                   VAssertDirectiveType directiveType) VL_MT_DISABLED;
    // Check an SVA value is within the implementation limit, else report it on nodep
    ایستا bool check_Sva_مقدار_Limit(Ast_گره* nodep, ثابت AstConst* constp,
                                   ثابت نویسه* what) VL_MT_DISABLED;
    // Fold the bounds of a cycle delay in a sequence, and check they are non-negative
    // elaboration-time constants (IEEE 1800-2023 16.7). If not, report it on errp, and replace
    // them with constants, so later steps do not fail after the error.
    ایستا تهی checkSvaCycleDelay(AstDelay* nodep, Ast_گره* errp) VL_MT_DISABLED;
    // True if the expression references a formal argument of a sequence or property
    ایستا bool refsSvaFormal(ثابت Ast_گره* nodep) VL_MT_DISABLED;
    // Check the cycle delays of a sequence or property instance, after its formal arguments are
    // replaced with the actual arguments (IEEE 1800-2023 16.8), reporting errors on instp
    ایستا تهی checkSvaInstanceDelays(Ast_گره* nodep, Ast_گره* instp) VL_MT_DISABLED;
};

class V3Assert final {
عمومی:
    ایستا تهی assertAll(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
