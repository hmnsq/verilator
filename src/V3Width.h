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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

class AstNetlist;
class Ast_ગાંઠ;
class Ast_ગાંઠ_Assign;
class Ast_ગાંઠ_DType;

//============================================================================

class V3Width final {
જાહેર:
    સ્થિર શૂન્ય width(AstNetlist* nodep) VL_MT_DISABLED;
    સ્થિર Ast_ગાંઠ* widthParamsEdit(Ast_ગાંઠ* nodep) VL_MT_DISABLED;
    સ્થિર Ast_ગાંઠ* widthGenerateParamsEdit(Ast_ગાંઠ* nodep) VL_MT_DISABLED;
    સ્થિર Ast_ગાંઠ* selectNonConstantRecurse(Ast_ગાંઠ* nodep, bool inSel = false) VL_MT_DISABLED;
    સ્થિર શૂન્ય streamAssignLowerEdit(Ast_ગાંઠ_Assign* nodep) VL_MT_DISABLED;

    // For use only in WidthVisitor
    // Replace AstSelBit, etc with AstSel/AstArraySel
    // Returns replacement node if nodep was deleted, or null if none.
    સ્થિર Ast_ગાંઠ* widthSelNoIterEdit(Ast_ગાંઠ* nodep) VL_MT_DISABLED;
};

#endif  // Guard
