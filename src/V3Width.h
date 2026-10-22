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

#شامل "config_build.h"
#شامل "verilatedos.h"

class AstNetlist;
class Ast_گره;
class Ast_گره_Assign;
class Ast_گره_DType;

//============================================================================

class V3Width final {
عمومی:
    ایستا تهی width(AstNetlist* nodep) VL_MT_DISABLED;
    ایستا Ast_گره* widthParamsEdit(Ast_گره* nodep) VL_MT_DISABLED;
    ایستا Ast_گره* widthGenerateParamsEdit(Ast_گره* nodep) VL_MT_DISABLED;
    ایستا Ast_گره* selectNonConstantRecurse(Ast_گره* nodep, bool inSel = false) VL_MT_DISABLED;
    ایستا تهی streamAssignLowerEdit(Ast_گره_Assign* nodep) VL_MT_DISABLED;

    // For use only in WidthVisitor
    // Replace AstSelBit, etc with AstSel/AstArraySel
    // Returns replacement node if nodep was deleted, or null if none.
    ایستا Ast_گره* widthSelNoIterEdit(Ast_گره* nodep) VL_MT_DISABLED;
};

#endif  // Guard
