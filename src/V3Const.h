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

#اگر_تعریف_نشده VERILATOR_V3CONST_H_
#تعریف_کن VERILATOR_V3CONST_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"

//============================================================================

class V3Const final {
عام:
    ساكن فراغ constifyParamsEdit(Ast_گره* nodep) VL_MT_DISABLED;
    ساكن فراغ constifyParamsNoWarnEdit(Ast_گره* nodep) VL_MT_DISABLED;
    ساكن فراغ constifyGenerateParamsEdit(Ast_گره* nodep) VL_MT_DISABLED;
    // Only do constant pushing, without removing dead logic
    ساكن فراغ constify_All_زنده(AstNetlist* nodep) VL_MT_DISABLED;
    // Everything that's possible
    ساكن فراغ constifyAll(AstNetlist* nodep) VL_MT_DISABLED;
    // Also, warn
    ساكن فراغ constifyAllLint(AstNetlist* nodep) VL_MT_DISABLED;
    // C++ datatypes
    ساكن فراغ constifyCpp(AstNetlist* nodep) VL_MT_DISABLED;
    // Only the current node and lower
    // Return new node that may have replaced nodep
    ساكن Ast_گره* constifyEditCpp(Ast_گره* nodep) VL_MT_DISABLED;
    ساكن Ast_گره_عبارت* constifyEditCpp(Ast_گره_عبارت* exprp) VL_MT_DISABLED {
        أرجع VN_AS(constifyEditCpp(static_cast<Ast_گره*>(exprp)), گره_عبارت);
    }
    // Only the current node and lower
    // Return new node that may have replaced nodep
    ساكن Ast_گره* constifyEdit(Ast_گره* nodep) VL_MT_DISABLED;
    ساكن Ast_گره_عبارت* constifyEdit(Ast_گره_عبارت* exprp) VL_MT_DISABLED {
        أرجع VN_AS(constifyEdit(static_cast<Ast_گره*>(exprp)), گره_عبارت);
    }
    // Only the current node and lower, with special SenTree optimization
    // Return new node that may have replaced nodep
    ساكن Ast_گره* constifyExpensiveEdit(Ast_گره* nodep) VL_MT_DISABLED;
};

#پایان_اگر  // Guard
