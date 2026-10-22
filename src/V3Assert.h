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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"

//============================================================================

class V3AssertCommon final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ collectDefaultDisable(AstNetlist* nodep) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ lowerSequenceEvents(AstNetlist* nodep) VL_MT_DISABLED;
    ಸ್ಥಿರ string assertCtlGetCall(ಧ್ರುವ ವರ್ಣ* query, VAssertType type,
                                   VAssertDirectiveType directiveType) VL_MT_DISABLED;
    // Check an SVA value is within the implementation limit, else report it on nodep
    ಸ್ಥಿರ bool check_Sva_ಮೌಲ್ಯ_Limit(Ast_ಗಂಟು* nodep, ಧ್ರುವ AstConst* constp,
                                   ಧ್ರುವ ವರ್ಣ* what) VL_MT_DISABLED;
    // Fold the bounds of a cycle delay in a sequence, and check they are non-negative
    // elaboration-time constants (IEEE 1800-2023 16.7). If not, report it on errp, and replace
    // them with constants, so later steps do not fail after the error.
    ಸ್ಥಿರ ಶೂನ್ಯ checkSvaCycleDelay(AstDelay* nodep, Ast_ಗಂಟು* errp) VL_MT_DISABLED;
    // True if the expression references a formal argument of a sequence or property
    ಸ್ಥಿರ bool refsSvaFormal(ಧ್ರುವ Ast_ಗಂಟು* nodep) VL_MT_DISABLED;
    // Check the cycle delays of a sequence or property instance, after its formal arguments are
    // replaced with the actual arguments (IEEE 1800-2023 16.8), reporting errors on instp
    ಸ್ಥಿರ ಶೂನ್ಯ checkSvaInstanceDelays(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* instp) VL_MT_DISABLED;
};

class V3Assert final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ assertAll(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
