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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"

//============================================================================

class V3Const final {
ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ constifyParamsEdit(Ast_ଗଣ୍ଠି* nodep) VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ constifyParamsNoWarnEdit(Ast_ଗଣ୍ଠି* nodep) VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ constifyGenerateParamsEdit(Ast_ଗଣ୍ଠି* nodep) VL_MT_DISABLED;
    // Only do constant pushing, without removing dead logic
    ସ୍ଥିର ଶୂନ୍ୟ constifyAllLive(AstNetlist* nodep) VL_MT_DISABLED;
    // Everything that's possible
    ସ୍ଥିର ଶୂନ୍ୟ constifyAll(AstNetlist* nodep) VL_MT_DISABLED;
    // Also, warn
    ସ୍ଥିର ଶୂନ୍ୟ constifyAllLint(AstNetlist* nodep) VL_MT_DISABLED;
    // C++ datatypes
    ସ୍ଥିର ଶୂନ୍ୟ constifyCpp(AstNetlist* nodep) VL_MT_DISABLED;
    // Only the current node and lower
    // Return new node that may have replaced nodep
    ସ୍ଥିର Ast_ଗଣ୍ଠି* constifyEditCpp(Ast_ଗଣ୍ଠି* nodep) VL_MT_DISABLED;
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* constifyEditCpp(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) VL_MT_DISABLED {
        ଫେରନ୍ତୁ VN_AS(constifyEditCpp(static_cast<Ast_ଗଣ୍ଠି*>(exprp)), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
    }
    // Only the current node and lower
    // Return new node that may have replaced nodep
    ସ୍ଥିର Ast_ଗଣ୍ଠି* constifyEdit(Ast_ଗଣ୍ଠି* nodep) VL_MT_DISABLED;
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* constifyEdit(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) VL_MT_DISABLED {
        ଫେରନ୍ତୁ VN_AS(constifyEdit(static_cast<Ast_ଗଣ୍ଠି*>(exprp)), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
    }
    // Only the current node and lower, with special SenTree optimization
    // Return new node that may have replaced nodep
    ସ୍ଥିର Ast_ଗଣ୍ଠି* constifyExpensiveEdit(Ast_ଗଣ୍ଠି* nodep) VL_MT_DISABLED;
};

#endif  // Guard
