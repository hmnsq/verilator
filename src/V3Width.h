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

#ifndef VERILATOR_V3WIDTH_H_
#define VERILATOR_V3WIDTH_H_

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

class AstNetlist;
class Ast_গ্রন্থি;
class Ast_গ্রন্থি_Assign;
class Ast_গ্রন্থি_DType;

//============================================================================

class V3Width final {
সর্বজনীন:
    স্থির শূন্য width(AstNetlist* nodep) VL_MT_DISABLED;
    স্থির Ast_গ্রন্থি* widthParamsEdit(Ast_গ্রন্থি* nodep) VL_MT_DISABLED;
    স্থির Ast_গ্রন্থি* widthGenerateParamsEdit(Ast_গ্রন্থি* nodep) VL_MT_DISABLED;
    স্থির Ast_গ্রন্থি* selectNonConstantRecurse(Ast_গ্রন্থি* nodep, bool inSel = false) VL_MT_DISABLED;
    স্থির শূন্য streamAssignLowerEdit(Ast_গ্রন্থি_Assign* nodep) VL_MT_DISABLED;

    // For use only in WidthVisitor
    // Replace AstSelBit, etc with AstSel/AstArraySel
    // Returns replacement node if nodep was deleted, or null if none.
    স্থির Ast_গ্রন্থি* widthSelNoIterEdit(Ast_গ্রন্থি* nodep) VL_MT_DISABLED;
};

#endif  // Guard
