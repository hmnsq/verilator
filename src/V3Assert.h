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

#अगर_अघोषित VERILATOR_V3ASSERT_H_
#घोषणा VERILATOR_V3ASSERT_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"

//============================================================================

class V3AssertCommon final {
खुला:
    अटल व्योम collectDefaultDisable(AstNetlist* nodep) VL_MT_DISABLED;
    अटल व्योम lowerSequenceEvents(AstNetlist* nodep) VL_MT_DISABLED;
    अटल string assertCtlGetCall(स्थिर अक्षर* query, VAssertType type,
                                   VAssertDirectiveType directiveType) VL_MT_DISABLED;
    // Check an SVA value is within the implementation limit, else report it on nodep
    अटल bool check_Sva_मूल्य_Limit(Ast_गाँठ* nodep, स्थिर AstConst* constp,
                                   स्थिर अक्षर* what) VL_MT_DISABLED;
    // Fold the bounds of a cycle delay in a sequence, and check they are non-negative
    // elaboration-time constants (IEEE 1800-2023 16.7). If not, report it on errp, and replace
    // them with constants, so later steps do not fail after the error.
    अटल व्योम checkSvaCycleDelay(AstDelay* nodep, Ast_गाँठ* errp) VL_MT_DISABLED;
    // True if the expression references a formal argument of a sequence or property
    अटल bool refsSvaFormal(स्थिर Ast_गाँठ* nodep) VL_MT_DISABLED;
    // Check the cycle delays of a sequence or property instance, after its formal arguments are
    // replaced with the actual arguments (IEEE 1800-2023 16.8), reporting errors on instp
    अटल व्योम checkSvaInstanceDelays(Ast_गाँठ* nodep, Ast_गाँठ* instp) VL_MT_DISABLED;
};

class V3Assert final {
खुला:
    अटल व्योम assertAll(AstNetlist* nodep) VL_MT_DISABLED;
};

#पूर्ण_अगर  // Guard
