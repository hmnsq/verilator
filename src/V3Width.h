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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3_ويڪر_H
#وصف_ڪريو VERILATOR_V3_ويڪر_H

#ضمن "config_build.h"
#ضمن "verilatedos.h"

class AstNetlist;
class Ast_ڳنڍ;
class Ast_ڳنڍ_سونپيو;
class Ast_ڳنڍ_DType;

//============================================================================

class V3_ويڪر final {
عام:
    ساكن فراغ ويڪر(AstNetlist* nodep) VL_MT_DISABLED;
    ساكن Ast_ڳنڍ* ويڪر_Params_Edit(Ast_ڳنڍ* nodep) VL_MT_DISABLED;
    ساكن Ast_ڳنڍ* ويڪر_Generate_Params_Edit(Ast_ڳنڍ* nodep) VL_MT_DISABLED;
    ساكن Ast_ڳنڍ* selectNonConstantRecurse(Ast_ڳنڍ* nodep, bool اندريون_Sel = false) VL_MT_DISABLED;
    ساكن فراغ وهڪرو_سونپيو_هيٺاهون_Edit(Ast_ڳنڍ_سونپيو* nodep) VL_MT_DISABLED;

    // For use only in WidthVisitor
    // Replace AstSelBit, etc with AstSel/AstArraySel
    // Returns replacement node if nodep was deleted, or null if none.
    ساكن Ast_ڳنڍ* ويڪر_Sel_No_Iter_Edit(Ast_ڳنڍ* nodep) VL_MT_DISABLED;
};

#جيڪڏهن_پورو  // Guard
