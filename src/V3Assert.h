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

#اگر_غیر_تعریف_شدہ VERILATOR_V3ASSERT_H_
#تعریف_کرو VERILATOR_V3ASSERT_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"

//============================================================================

class V3AssertCommon final {
عوامی:
    ساکن خالی collectDefaultDisable(AstNetlist* nodep) VL_MT_DISABLED;
    ساکن خالی زیریں_Sequence_Events(AstNetlist* nodep) VL_MT_DISABLED;
    ساکن string assertCtlGetCall(ثابت حرف* query, VAssertType type,
                                   VAssertDirectiveType directiveType) VL_MT_DISABLED;
    // Check an SVA value is within the implementation limit, else report it on nodep
    ساکن bool check_Sva_قدر_حد_بندی(Ast_گرہ* nodep, ثابت AstConst* constp,
                                   ثابت حرف* what) VL_MT_DISABLED;
    // Fold the bounds of a cycle delay in a sequence, and check they are non-negative
    // elaboration-time constants (IEEE 1800-2023 16.7). If not, report it on errp, and replace
    // them with constants, so later steps do not fail after the error.
    ساکن خالی check_Sva_چکر_Delay(AstDelay* nodep, Ast_گرہ* errp) VL_MT_DISABLED;
    // True if the expression references a formal argument of a sequence or property
    ساکن bool refsSvaFormal(ثابت Ast_گرہ* nodep) VL_MT_DISABLED;
    // Check the cycle delays of a sequence or property instance, after its formal arguments are
    // replaced with the actual arguments (IEEE 1800-2023 16.8), reporting errors on instp
    ساکن خالی checkSvaInstanceDelays(Ast_گرہ* nodep, Ast_گرہ* instp) VL_MT_DISABLED;
};

class V3Assert final {
عوامی:
    ساکن خالی assertAll(AstNetlist* nodep) VL_MT_DISABLED;
};

#اگر_ختم  // Guard
