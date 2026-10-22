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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

class AstNetlist;
class Ast_முனை;
class Ast_முனை_Assign;
class Ast_முனை_DType;

//============================================================================

class V3Width final {
பொது:
    நிலையான வெறுமை width(AstNetlist* nodep) VL_MT_DISABLED;
    நிலையான Ast_முனை* widthParamsEdit(Ast_முனை* nodep) VL_MT_DISABLED;
    நிலையான Ast_முனை* widthGenerateParamsEdit(Ast_முனை* nodep) VL_MT_DISABLED;
    நிலையான Ast_முனை* selectNonConstantRecurse(Ast_முனை* nodep, bool inSel = false) VL_MT_DISABLED;
    நிலையான வெறுமை streamAssignLowerEdit(Ast_முனை_Assign* nodep) VL_MT_DISABLED;

    // For use only in WidthVisitor
    // Replace AstSelBit, etc with AstSel/AstArraySel
    // Returns replacement node if nodep was deleted, or null if none.
    நிலையான Ast_முனை* widthSelNoIterEdit(Ast_முனை* nodep) VL_MT_DISABLED;
};

#endif  // Guard
