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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"

//============================================================================

class V3AssertCommon final {
सार्वजनिक:
    स्थिर शून्य collectDefaultDisable(AstNetlist* nodep) VL_MT_DISABLED;
    स्थिर शून्य lowerSequenceEvents(AstNetlist* nodep) VL_MT_DISABLED;
    स्थिर string assertCtlGetCall(ध्रुव वर्ण* query, VAssertType type,
                                   VAssertDirectiveType directiveType) VL_MT_DISABLED;
    // Check an SVA value is within the implementation limit, else report it on nodep
    स्थिर bool check_Sva_मूल्य_Limit(Ast_गाँठ* nodep, ध्रुव AstConst* constp,
                                   ध्रुव वर्ण* what) VL_MT_DISABLED;
    // Fold the bounds of a cycle delay in a sequence, and check they are non-negative
    // elaboration-time constants (IEEE 1800-2023 16.7). If not, report it on errp, and replace
    // them with constants, so later steps do not fail after the error.
    स्थिर शून्य checkSvaCycleDelay(AstDelay* nodep, Ast_गाँठ* errp) VL_MT_DISABLED;
    // True if the expression references a formal argument of a sequence or property
    स्थिर bool refsSvaFormal(ध्रुव Ast_गाँठ* nodep) VL_MT_DISABLED;
    // Check the cycle delays of a sequence or property instance, after its formal arguments are
    // replaced with the actual arguments (IEEE 1800-2023 16.8), reporting errors on instp
    स्थिर शून्य checkSvaInstanceDelays(Ast_गाँठ* nodep, Ast_गाँठ* instp) VL_MT_DISABLED;
};

class V3Assert final {
सार्वजनिक:
    स्थिर शून्य assertAll(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
