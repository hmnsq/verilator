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

#include "config_build.h"
#include "verilatedos.h"

#include "V3Ast.h"

//============================================================================

class V3Const final {
public:
    static void constifyParamsEdit(Ast_बुहोल* nodep) VL_MT_DISABLED;
    static void constifyParamsNoWarnEdit(Ast_बुहोल* nodep) VL_MT_DISABLED;
    static void constifyGenerateParamsEdit(Ast_बुहोल* nodep) VL_MT_DISABLED;
    // Only do constant pushing, without removing dead logic
    static void constifyAllLive(AstNetlist* nodep) VL_MT_DISABLED;
    // Everything that's possible
    static void constifyAll(AstNetlist* nodep) VL_MT_DISABLED;
    // Also, warn
    static void constifyAllLint(AstNetlist* nodep) VL_MT_DISABLED;
    // C++ datatypes
    static void constifyCpp(AstNetlist* nodep) VL_MT_DISABLED;
    // Only the current node and lower
    // Return new node that may have replaced nodep
    static Ast_बुहोल* constifyEditCpp(Ast_बुहोल* nodep) VL_MT_DISABLED;
    static Ast_बुहोल_पाहायाग* constifyEditCpp(Ast_बुहोल_पाहायाग* exprp) VL_MT_DISABLED {
        return VN_AS(constifyEditCpp(static_cast<Ast_बुहोल*>(exprp)), बुहोल_पाहायाग);
    }
    // Only the current node and lower
    // Return new node that may have replaced nodep
    static Ast_बुहोल* constifyEdit(Ast_बुहोल* nodep) VL_MT_DISABLED;
    static Ast_बुहोल_पाहायाग* constifyEdit(Ast_बुहोल_पाहायाग* exprp) VL_MT_DISABLED {
        return VN_AS(constifyEdit(static_cast<Ast_बुहोल*>(exprp)), बुहोल_पाहायाग);
    }
    // Only the current node and lower, with special SenTree optimization
    // Return new node that may have replaced nodep
    static Ast_बुहोल* constifyExpensiveEdit(Ast_बुहोल* nodep) VL_MT_DISABLED;
};

#endif  // Guard
