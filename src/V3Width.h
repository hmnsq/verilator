// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Node attributes/ expression widths
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

#اگر_تعریف_نشده VERILATOR_V3_عرض_H
#تعریف_کن VERILATOR_V3_عرض_H

#ضمن "config_build.h"
#ضمن "verilatedos.h"

class AstNetlist;
class Ast_گره;
class Ast_گره_تخصیص_بده;
class Ast_گره_DType;

//============================================================================

class V3_عرض final {
عام:
    ساكن فراغ عرض(AstNetlist* nodep) VL_MT_DISABLED;
    ساكن Ast_گره* عرض_Params_Edit(Ast_گره* nodep) VL_MT_DISABLED;
    ساكن Ast_گره* عرض_Generate_Params_Edit(Ast_گره* nodep) VL_MT_DISABLED;
    ساكن Ast_گره* selectNonConstantRecurse(Ast_گره* nodep, bool درون_Sel = false) VL_MT_DISABLED;
    ساكن فراغ جریان_تخصیص_بده_پایینی_Edit(Ast_گره_تخصیص_بده* nodep) VL_MT_DISABLED;

    // For use only in WidthVisitor
    // Replace AstSelBit, etc with AstSel/AstArraySel
    // Returns replacement node if nodep was deleted, or null if none.
    ساكن Ast_گره* عرض_Sel_No_Iter_Edit(Ast_گره* nodep) VL_MT_DISABLED;
};

#پایان_اگر  // Guard
