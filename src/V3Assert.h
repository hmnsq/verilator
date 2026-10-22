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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3ASSERT_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3ASSERT_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"

//============================================================================

class V3AssertCommon final {
ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ collectDefaultDisable(AstNetlist* nodep) VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ ਹੇਠਲਾ_Sequence_Events(AstNetlist* nodep) VL_MT_DISABLED;
    ਸਥਿਰ string assertCtlGetCall(ਅਚਲ ਅੱਖਰ* query, VAssertType type,
                                   VAssertDirectiveType directiveType) VL_MT_DISABLED;
    // Check an SVA value is within the implementation limit, else report it on nodep
    ਸਥਿਰ bool check_Sva_ਮੁੱਲ_ਹੱਦਬੰਦੀ(Ast_ਗੰਢ* nodep, ਅਚਲ AstConst* constp,
                                   ਅਚਲ ਅੱਖਰ* what) VL_MT_DISABLED;
    // Fold the bounds of a cycle delay in a sequence, and check they are non-negative
    // elaboration-time constants (IEEE 1800-2023 16.7). If not, report it on errp, and replace
    // them with constants, so later steps do not fail after the error.
    ਸਥਿਰ ਖਾਲੀ check_Sva_ਚੱਕਰ_Delay(AstDelay* nodep, Ast_ਗੰਢ* errp) VL_MT_DISABLED;
    // True if the expression references a formal argument of a sequence or property
    ਸਥਿਰ bool refsSvaFormal(ਅਚਲ Ast_ਗੰਢ* nodep) VL_MT_DISABLED;
    // Check the cycle delays of a sequence or property instance, after its formal arguments are
    // replaced with the actual arguments (IEEE 1800-2023 16.8), reporting errors on instp
    ਸਥਿਰ ਖਾਲੀ checkSvaInstanceDelays(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* instp) VL_MT_DISABLED;
};

class V3Assert final {
ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ assertAll(AstNetlist* nodep) VL_MT_DISABLED;
};

#ਜੇ_ਅੰਤ  // Guard
