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

#ifndef VERILATOR_V3CONST_H_
#define VERILATOR_V3CONST_H_

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"

//============================================================================

class V3Const final {
பொது:
    நிலையான வெறுமை constifyParamsEdit(Ast_முனை* nodep) VL_MT_DISABLED;
    நிலையான வெறுமை constifyParamsNoWarnEdit(Ast_முனை* nodep) VL_MT_DISABLED;
    நிலையான வெறுமை constifyGenerateParamsEdit(Ast_முனை* nodep) VL_MT_DISABLED;
    // Only do constant pushing, without removing dead logic
    நிலையான வெறுமை constifyAllLive(AstNetlist* nodep) VL_MT_DISABLED;
    // Everything that's possible
    நிலையான வெறுமை constifyAll(AstNetlist* nodep) VL_MT_DISABLED;
    // Also, warn
    நிலையான வெறுமை constifyAllLint(AstNetlist* nodep) VL_MT_DISABLED;
    // C++ datatypes
    நிலையான வெறுமை constifyCpp(AstNetlist* nodep) VL_MT_DISABLED;
    // Only the current node and lower
    // Return new node that may have replaced nodep
    நிலையான Ast_முனை* constifyEditCpp(Ast_முனை* nodep) VL_MT_DISABLED;
    நிலையான Ast_முனை_கோவை* constifyEditCpp(Ast_முனை_கோவை* exprp) VL_MT_DISABLED {
        திரும்பு VN_AS(constifyEditCpp(static_cast<Ast_முனை*>(exprp)), முனை_கோவை);
    }
    // Only the current node and lower
    // Return new node that may have replaced nodep
    நிலையான Ast_முனை* constifyEdit(Ast_முனை* nodep) VL_MT_DISABLED;
    நிலையான Ast_முனை_கோவை* constifyEdit(Ast_முனை_கோவை* exprp) VL_MT_DISABLED {
        திரும்பு VN_AS(constifyEdit(static_cast<Ast_முனை*>(exprp)), முனை_கோவை);
    }
    // Only the current node and lower, with special SenTree optimization
    // Return new node that may have replaced nodep
    நிலையான Ast_முனை* constifyExpensiveEdit(Ast_முனை* nodep) VL_MT_DISABLED;
};

#endif  // Guard
