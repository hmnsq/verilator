// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Propagate constants across AST
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

#यदि_अपरिभाषितम् VERILATOR_V3CONST_H_
#परिभाषय VERILATOR_V3CONST_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"

//============================================================================

class V3Const final {
सार्वजनिक:
    स्थिर शून्य constifyParamsEdit(Ast_ग्रन्थिः* nodep) VL_MT_DISABLED;
    स्थिर शून्य constifyParamsNoWarnEdit(Ast_ग्रन्थिः* nodep) VL_MT_DISABLED;
    स्थिर शून्य constifyGenerateParamsEdit(Ast_ग्रन्थिः* nodep) VL_MT_DISABLED;
    // Only do constant pushing, without removing dead logic
    स्थिर शून्य constify_All_जीवितम्(AstNetlist* nodep) VL_MT_DISABLED;
    // Everything that's possible
    स्थिर शून्य constifyAll(AstNetlist* nodep) VL_MT_DISABLED;
    // Also, warn
    स्थिर शून्य constifyAllLint(AstNetlist* nodep) VL_MT_DISABLED;
    // C++ datatypes
    स्थिर शून्य constifyCpp(AstNetlist* nodep) VL_MT_DISABLED;
    // Only the current node and lower
    // Return new node that may have replaced nodep
    स्थिर Ast_ग्रन्थिः* constifyEditCpp(Ast_ग्रन्थिः* nodep) VL_MT_DISABLED;
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* constifyEditCpp(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) VL_MT_DISABLED {
        प्रत्यागम VN_AS(constifyEditCpp(static_cast<Ast_ग्रन्थिः*>(exprp)), ग्रन्थिः_व्यञ्जकम्);
    }
    // Only the current node and lower
    // Return new node that may have replaced nodep
    स्थिर Ast_ग्रन्थिः* constifyEdit(Ast_ग्रन्थिः* nodep) VL_MT_DISABLED;
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* constifyEdit(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) VL_MT_DISABLED {
        प्रत्यागम VN_AS(constifyEdit(static_cast<Ast_ग्रन्थिः*>(exprp)), ग्रन्थिः_व्यञ्जकम्);
    }
    // Only the current node and lower, with special SenTree optimization
    // Return new node that may have replaced nodep
    स्थिर Ast_ग्रन्थिः* constifyExpensiveEdit(Ast_ग्रन्थिः* nodep) VL_MT_DISABLED;
};

#यदि_अन्तः  // Guard
