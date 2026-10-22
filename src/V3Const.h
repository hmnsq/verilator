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

#నిర్వచితం_కాకపోతే VERILATOR_V3CONST_H_
#నిర్వచించు VERILATOR_V3CONST_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"

//============================================================================

class V3Const final {
బహిరంగ:
    స్థిర శూన్య constifyParamsEdit(Ast_కణుపు* nodep) VL_MT_DISABLED;
    స్థిర శూన్య constifyParamsNoWarnEdit(Ast_కణుపు* nodep) VL_MT_DISABLED;
    స్థిర శూన్య constifyGenerateParamsEdit(Ast_కణుపు* nodep) VL_MT_DISABLED;
    // Only do constant pushing, without removing dead logic
    స్థిర శూన్య constify_All_సజీవ(AstNetlist* nodep) VL_MT_DISABLED;
    // Everything that's possible
    స్థిర శూన్య constifyAll(AstNetlist* nodep) VL_MT_DISABLED;
    // Also, warn
    స్థిర శూన్య constifyAllLint(AstNetlist* nodep) VL_MT_DISABLED;
    // C++ datatypes
    స్థిర శూన్య constifyCpp(AstNetlist* nodep) VL_MT_DISABLED;
    // Only the current node and lower
    // Return new node that may have replaced nodep
    స్థిర Ast_కణుపు* constifyEditCpp(Ast_కణుపు* nodep) VL_MT_DISABLED;
    స్థిర Ast_కణుపు_సమాసం* constifyEditCpp(Ast_కణుపు_సమాసం* exprp) VL_MT_DISABLED {
        తిరిగి VN_AS(constifyEditCpp(static_cast<Ast_కణుపు*>(exprp)), కణుపు_సమాసం);
    }
    // Only the current node and lower
    // Return new node that may have replaced nodep
    స్థిర Ast_కణుపు* constifyEdit(Ast_కణుపు* nodep) VL_MT_DISABLED;
    స్థిర Ast_కణుపు_సమాసం* constifyEdit(Ast_కణుపు_సమాసం* exprp) VL_MT_DISABLED {
        తిరిగి VN_AS(constifyEdit(static_cast<Ast_కణుపు*>(exprp)), కణుపు_సమాసం);
    }
    // Only the current node and lower, with special SenTree optimization
    // Return new node that may have replaced nodep
    స్థిర Ast_కణుపు* constifyExpensiveEdit(Ast_కణుపు* nodep) VL_MT_DISABLED;
};

#అయితే_ముగింపు  // Guard
