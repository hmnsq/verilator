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

#جے_بنا_تعریف VERILATOR_V3CONST_H_
#تعریف_کرو VERILATOR_V3CONST_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"

//============================================================================

class V3Const final {
عام:
    ساكن فراغ constifyParamsEdit(Ast_گرہ* nodep) VL_MT_DISABLED;
    ساكن فراغ constifyParamsNoWarnEdit(Ast_گرہ* nodep) VL_MT_DISABLED;
    ساكن فراغ constifyGenerateParamsEdit(Ast_گرہ* nodep) VL_MT_DISABLED;
    // Only do constant pushing, without removing dead logic
    ساكن فراغ constify_All_جیوندا(AstNetlist* nodep) VL_MT_DISABLED;
    // Everything that's possible
    ساكن فراغ constifyAll(AstNetlist* nodep) VL_MT_DISABLED;
    // Also, warn
    ساكن فراغ constifyAllLint(AstNetlist* nodep) VL_MT_DISABLED;
    // C++ datatypes
    ساكن فراغ constifyCpp(AstNetlist* nodep) VL_MT_DISABLED;
    // Only the current node and lower
    // Return new node that may have replaced nodep
    ساكن Ast_گرہ* constifyEditCpp(Ast_گرہ* nodep) VL_MT_DISABLED;
    ساكن Ast_گرہ_اظہاریہ* constifyEditCpp(Ast_گرہ_اظہاریہ* exprp) VL_MT_DISABLED {
        أرجع VN_AS(constifyEditCpp(static_cast<Ast_گرہ*>(exprp)), گرہ_اظہاریہ);
    }
    // Only the current node and lower
    // Return new node that may have replaced nodep
    ساكن Ast_گرہ* constifyEdit(Ast_گرہ* nodep) VL_MT_DISABLED;
    ساكن Ast_گرہ_اظہاریہ* constifyEdit(Ast_گرہ_اظہاریہ* exprp) VL_MT_DISABLED {
        أرجع VN_AS(constifyEdit(static_cast<Ast_گرہ*>(exprp)), گرہ_اظہاریہ);
    }
    // Only the current node and lower, with special SenTree optimization
    // Return new node that may have replaced nodep
    ساكن Ast_گرہ* constifyExpensiveEdit(Ast_گرہ* nodep) VL_MT_DISABLED;
};

#جے_انت  // Guard
