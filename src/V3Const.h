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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3CONST_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3CONST_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"

//============================================================================

class V3Const final {
ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ constifyParamsEdit(Ast_ਗੰਢ* nodep) VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ constifyParamsNoWarnEdit(Ast_ਗੰਢ* nodep) VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ constifyGenerateParamsEdit(Ast_ਗੰਢ* nodep) VL_MT_DISABLED;
    // Only do constant pushing, without removing dead logic
    ਸਥਿਰ ਖਾਲੀ constify_All_ਜੀਵਤ(AstNetlist* nodep) VL_MT_DISABLED;
    // Everything that's possible
    ਸਥਿਰ ਖਾਲੀ constifyAll(AstNetlist* nodep) VL_MT_DISABLED;
    // Also, warn
    ਸਥਿਰ ਖਾਲੀ constifyAllLint(AstNetlist* nodep) VL_MT_DISABLED;
    // C++ datatypes
    ਸਥਿਰ ਖਾਲੀ constifyCpp(AstNetlist* nodep) VL_MT_DISABLED;
    // Only the current node and lower
    // Return new node that may have replaced nodep
    ਸਥਿਰ Ast_ਗੰਢ* constifyEditCpp(Ast_ਗੰਢ* nodep) VL_MT_DISABLED;
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* constifyEditCpp(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) VL_MT_DISABLED {
        ਵਾਪਸ VN_AS(constifyEditCpp(static_cast<Ast_ਗੰਢ*>(exprp)), ਗੰਢ_ਵਿਅੰਜਕ);
    }
    // Only the current node and lower
    // Return new node that may have replaced nodep
    ਸਥਿਰ Ast_ਗੰਢ* constifyEdit(Ast_ਗੰਢ* nodep) VL_MT_DISABLED;
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* constifyEdit(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) VL_MT_DISABLED {
        ਵਾਪਸ VN_AS(constifyEdit(static_cast<Ast_ਗੰਢ*>(exprp)), ਗੰਢ_ਵਿਅੰਜਕ);
    }
    // Only the current node and lower, with special SenTree optimization
    // Return new node that may have replaced nodep
    ਸਥਿਰ Ast_ਗੰਢ* constifyExpensiveEdit(Ast_ਗੰਢ* nodep) VL_MT_DISABLED;
};

#ਜੇ_ਅੰਤ  // Guard
