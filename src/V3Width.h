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

#جے_بنا_تعریف VERILATOR_V3_چوڑائی_H
#تعریف_کرو VERILATOR_V3_چوڑائی_H

#ضمن "config_build.h"
#ضمن "verilatedos.h"

class AstNetlist;
class Ast_گرہ;
class Ast_گرہ_سونپو;
class Ast_گرہ_DType;

//============================================================================

class V3_چوڑائی final {
عام:
    ساكن فراغ چوڑائی(AstNetlist* nodep) VL_MT_DISABLED;
    ساكن Ast_گرہ* چوڑائی_Params_Edit(Ast_گرہ* nodep) VL_MT_DISABLED;
    ساكن Ast_گرہ* چوڑائی_Generate_Params_Edit(Ast_گرہ* nodep) VL_MT_DISABLED;
    ساكن Ast_گرہ* selectNonConstantRecurse(Ast_گرہ* nodep, bool وچ_Sel = false) VL_MT_DISABLED;
    ساكن فراغ دھارا_سونپو_ہیٹھلا_Edit(Ast_گرہ_سونپو* nodep) VL_MT_DISABLED;

    // For use only in WidthVisitor
    // Replace AstSelBit, etc with AstSel/AstArraySel
    // Returns replacement node if nodep was deleted, or null if none.
    ساكن Ast_گرہ* چوڑائی_Sel_No_Iter_Edit(Ast_گرہ* nodep) VL_MT_DISABLED;
};

#جے_انت  // Guard
