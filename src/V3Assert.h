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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Ast.h"

//============================================================================

class V3AssertCommon final {
সার্ৱজনিক:
    স্থির শূন্য collectDefaultDisable(AstNetlist* nodep) VL_MT_DISABLED;
    স্থির শূন্য lowerSequenceEvents(AstNetlist* nodep) VL_MT_DISABLED;
    স্থির string assertCtlGetCall(ধ্রুৱ ৱর্ণ* query, VAssertType type,
                                   VAssertDirectiveType directiveType) VL_MT_DISABLED;
    // Check an SVA value is within the implementation limit, else report it on nodep
    স্থির bool check_Sva_মান_Limit(Ast_গ্ৰন্থি* nodep, ধ্রুৱ AstConst* constp,
                                   ধ্রুৱ ৱর্ণ* what) VL_MT_DISABLED;
    // Fold the bounds of a cycle delay in a sequence, and check they are non-negative
    // elaboration-time constants (IEEE 1800-2023 16.7). If not, report it on errp, and replace
    // them with constants, so later steps do not fail after the error.
    স্থির শূন্য checkSvaCycleDelay(AstDelay* nodep, Ast_গ্ৰন্থি* errp) VL_MT_DISABLED;
    // True if the expression references a formal argument of a sequence or property
    স্থির bool refsSvaFormal(ধ্রুৱ Ast_গ্ৰন্থি* nodep) VL_MT_DISABLED;
    // Check the cycle delays of a sequence or property instance, after its formal arguments are
    // replaced with the actual arguments (IEEE 1800-2023 16.8), reporting errors on instp
    স্থির শূন্য checkSvaInstanceDelays(Ast_গ্ৰন্থি* nodep, Ast_গ্ৰন্থি* instp) VL_MT_DISABLED;
};

class V3Assert final {
সার্ৱজনিক:
    স্থির শূন্য assertAll(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
