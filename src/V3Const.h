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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Ast.h"

//============================================================================

class V3Const final {
সার্ৱজনিক:
    স্থির শূন্য constifyParamsEdit(Ast_গ্ৰন্থি* nodep) VL_MT_DISABLED;
    স্থির শূন্য constifyParamsNoWarnEdit(Ast_গ্ৰন্থি* nodep) VL_MT_DISABLED;
    স্থির শূন্য constifyGenerateParamsEdit(Ast_গ্ৰন্থি* nodep) VL_MT_DISABLED;
    // Only do constant pushing, without removing dead logic
    স্থির শূন্য constifyAllLive(AstNetlist* nodep) VL_MT_DISABLED;
    // Everything that's possible
    স্থির শূন্য constifyAll(AstNetlist* nodep) VL_MT_DISABLED;
    // Also, warn
    স্থির শূন্য constifyAllLint(AstNetlist* nodep) VL_MT_DISABLED;
    // C++ datatypes
    স্থির শূন্য constifyCpp(AstNetlist* nodep) VL_MT_DISABLED;
    // Only the current node and lower
    // Return new node that may have replaced nodep
    স্থির Ast_গ্ৰন্থি* constifyEditCpp(Ast_গ্ৰন্থি* nodep) VL_MT_DISABLED;
    স্থির Ast_গ্ৰন্থি_ৰাশিমালা* constifyEditCpp(Ast_গ্ৰন্থি_ৰাশিমালা* exprp) VL_MT_DISABLED {
        ঘূৰাই VN_AS(constifyEditCpp(static_cast<Ast_গ্ৰন্থি*>(exprp)), গ্ৰন্থি_ৰাশিমালা);
    }
    // Only the current node and lower
    // Return new node that may have replaced nodep
    স্থির Ast_গ্ৰন্থি* constifyEdit(Ast_গ্ৰন্থি* nodep) VL_MT_DISABLED;
    স্থির Ast_গ্ৰন্থি_ৰাশিমালা* constifyEdit(Ast_গ্ৰন্থি_ৰাশিমালা* exprp) VL_MT_DISABLED {
        ঘূৰাই VN_AS(constifyEdit(static_cast<Ast_গ্ৰন্থি*>(exprp)), গ্ৰন্থি_ৰাশিমালা);
    }
    // Only the current node and lower, with special SenTree optimization
    // Return new node that may have replaced nodep
    স্থির Ast_গ্ৰন্থি* constifyExpensiveEdit(Ast_গ্ৰন্থি* nodep) VL_MT_DISABLED;
};

#endif  // Guard
