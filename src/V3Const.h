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

#اگر_غیر_تعریف_شدہ VERILATOR_V3CONST_H_
#تعریف_کرو VERILATOR_V3CONST_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"

//============================================================================

class V3Const final {
عوامی:
    ساکن خالی constifyParamsEdit(Ast_گرہ* nodep) VL_MT_DISABLED;
    ساکن خالی constifyParamsNoWarnEdit(Ast_گرہ* nodep) VL_MT_DISABLED;
    ساکن خالی constifyGenerateParamsEdit(Ast_گرہ* nodep) VL_MT_DISABLED;
    // Only do constant pushing, without removing dead logic
    ساکن خالی constify_All_زندہ(AstNetlist* nodep) VL_MT_DISABLED;
    // Everything that's possible
    ساکن خالی constifyAll(AstNetlist* nodep) VL_MT_DISABLED;
    // Also, warn
    ساکن خالی constifyAllLint(AstNetlist* nodep) VL_MT_DISABLED;
    // C++ datatypes
    ساکن خالی constifyCpp(AstNetlist* nodep) VL_MT_DISABLED;
    // Only the current node and lower
    // Return new node that may have replaced nodep
    ساکن Ast_گرہ* constifyEditCpp(Ast_گرہ* nodep) VL_MT_DISABLED;
    ساکن Ast_گرہ_اظہاریہ* constifyEditCpp(Ast_گرہ_اظہاریہ* exprp) VL_MT_DISABLED {
        واپس VN_AS(constifyEditCpp(static_cast<Ast_گرہ*>(exprp)), گرہ_اظہاریہ);
    }
    // Only the current node and lower
    // Return new node that may have replaced nodep
    ساکن Ast_گرہ* constifyEdit(Ast_گرہ* nodep) VL_MT_DISABLED;
    ساکن Ast_گرہ_اظہاریہ* constifyEdit(Ast_گرہ_اظہاریہ* exprp) VL_MT_DISABLED {
        واپس VN_AS(constifyEdit(static_cast<Ast_گرہ*>(exprp)), گرہ_اظہاریہ);
    }
    // Only the current node and lower, with special SenTree optimization
    // Return new node that may have replaced nodep
    ساکن Ast_گرہ* constifyExpensiveEdit(Ast_گرہ* nodep) VL_MT_DISABLED;
};

#اگر_ختم  // Guard
