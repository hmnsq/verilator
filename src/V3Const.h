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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"

//============================================================================

class V3Const final {
જાહેર:
    સ્થિર શૂન્ય constifyParamsEdit(Ast_ગાંઠ* nodep) VL_MT_DISABLED;
    સ્થિર શૂન્ય constifyParamsNoWarnEdit(Ast_ગાંઠ* nodep) VL_MT_DISABLED;
    સ્થિર શૂન્ય constifyGenerateParamsEdit(Ast_ગાંઠ* nodep) VL_MT_DISABLED;
    // Only do constant pushing, without removing dead logic
    સ્થિર શૂન્ય constifyAllLive(AstNetlist* nodep) VL_MT_DISABLED;
    // Everything that's possible
    સ્થિર શૂન્ય constifyAll(AstNetlist* nodep) VL_MT_DISABLED;
    // Also, warn
    સ્થિર શૂન્ય constifyAllLint(AstNetlist* nodep) VL_MT_DISABLED;
    // C++ datatypes
    સ્થિર શૂન્ય constifyCpp(AstNetlist* nodep) VL_MT_DISABLED;
    // Only the current node and lower
    // Return new node that may have replaced nodep
    સ્થિર Ast_ગાંઠ* constifyEditCpp(Ast_ગાંઠ* nodep) VL_MT_DISABLED;
    સ્થિર Ast_ગાંઠ_પદાવલિ* constifyEditCpp(Ast_ગાંઠ_પદાવલિ* exprp) VL_MT_DISABLED {
        પાછા VN_AS(constifyEditCpp(static_cast<Ast_ગાંઠ*>(exprp)), ગાંઠ_પદાવલિ);
    }
    // Only the current node and lower
    // Return new node that may have replaced nodep
    સ્થિર Ast_ગાંઠ* constifyEdit(Ast_ગાંઠ* nodep) VL_MT_DISABLED;
    સ્થિર Ast_ગાંઠ_પદાવલિ* constifyEdit(Ast_ગાંઠ_પદાવલિ* exprp) VL_MT_DISABLED {
        પાછા VN_AS(constifyEdit(static_cast<Ast_ગાંઠ*>(exprp)), ગાંઠ_પદાવલિ);
    }
    // Only the current node and lower, with special SenTree optimization
    // Return new node that may have replaced nodep
    સ્થિર Ast_ગાંઠ* constifyExpensiveEdit(Ast_ગાંઠ* nodep) VL_MT_DISABLED;
};

#endif  // Guard
