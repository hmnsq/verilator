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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"

//============================================================================

class V3Const final {
പൊതു:
    സ്ഥിതം ശൂന്യം constifyParamsEdit(Ast_കണ്ണി* nodep) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം constifyParamsNoWarnEdit(Ast_കണ്ണി* nodep) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം constifyGenerateParamsEdit(Ast_കണ്ണി* nodep) VL_MT_DISABLED;
    // Only do constant pushing, without removing dead logic
    സ്ഥിതം ശൂന്യം constifyAllLive(AstNetlist* nodep) VL_MT_DISABLED;
    // Everything that's possible
    സ്ഥിതം ശൂന്യം constifyAll(AstNetlist* nodep) VL_MT_DISABLED;
    // Also, warn
    സ്ഥിതം ശൂന്യം constifyAllLint(AstNetlist* nodep) VL_MT_DISABLED;
    // C++ datatypes
    സ്ഥിതം ശൂന്യം constifyCpp(AstNetlist* nodep) VL_MT_DISABLED;
    // Only the current node and lower
    // Return new node that may have replaced nodep
    സ്ഥിതം Ast_കണ്ണി* constifyEditCpp(Ast_കണ്ണി* nodep) VL_MT_DISABLED;
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* constifyEditCpp(Ast_കണ്ണി_വ്യഞ്ജകം* exprp) VL_MT_DISABLED {
        തിരികെ VN_AS(constifyEditCpp(static_cast<Ast_കണ്ണി*>(exprp)), കണ്ണി_വ്യഞ്ജകം);
    }
    // Only the current node and lower
    // Return new node that may have replaced nodep
    സ്ഥിതം Ast_കണ്ണി* constifyEdit(Ast_കണ്ണി* nodep) VL_MT_DISABLED;
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* constifyEdit(Ast_കണ്ണി_വ്യഞ്ജകം* exprp) VL_MT_DISABLED {
        തിരികെ VN_AS(constifyEdit(static_cast<Ast_കണ്ണി*>(exprp)), കണ്ണി_വ്യഞ്ജകം);
    }
    // Only the current node and lower, with special SenTree optimization
    // Return new node that may have replaced nodep
    സ്ഥിതം Ast_കണ്ണി* constifyExpensiveEdit(Ast_കണ്ണി* nodep) VL_MT_DISABLED;
};

#endif  // Guard
