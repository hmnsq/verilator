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

#که_نه_تعریف_شوی VERILATOR_V3ASSERT_H_
#تعریف_کړه VERILATOR_V3ASSERT_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"

//============================================================================

class V3AssertCommon final {
عام:
    ساكن فراغ collectDefaultDisable(AstNetlist* nodep) VL_MT_DISABLED;
    ساكن فراغ ښکتنی_Sequence_Events(AstNetlist* nodep) VL_MT_DISABLED;
    ساكن string assertCtlGetCall(ثابت حرف* query, VAssertType type,
                                   VAssertDirectiveType directiveType) VL_MT_DISABLED;
    // Check an SVA value is within the implementation limit, else report it on nodep
    ساكن bool check_Sva_ارزښت_حد(Ast_غوټه* nodep, ثابت AstConst* constp,
                                   ثابت حرف* what) VL_MT_DISABLED;
    // Fold the bounds of a cycle delay in a sequence, and check they are non-negative
    // elaboration-time constants (IEEE 1800-2023 16.7). If not, report it on errp, and replace
    // them with constants, so later steps do not fail after the error.
    ساكن فراغ check_Sva_دوره_Delay(AstDelay* nodep, Ast_غوټه* errp) VL_MT_DISABLED;
    // True if the expression references a formal argument of a sequence or property
    ساكن bool refsSvaFormal(ثابت Ast_غوټه* nodep) VL_MT_DISABLED;
    // Check the cycle delays of a sequence or property instance, after its formal arguments are
    // replaced with the actual arguments (IEEE 1800-2023 16.8), reporting errors on instp
    ساكن فراغ checkSvaInstanceDelays(Ast_غوټه* nodep, Ast_غوټه* instp) VL_MT_DISABLED;
};

class V3Assert final {
عام:
    ساكن فراغ assertAll(AstNetlist* nodep) VL_MT_DISABLED;
};

#د_که_پای  // Guard
