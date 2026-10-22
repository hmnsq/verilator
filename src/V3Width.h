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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

class AstNetlist;
class Ast_عقدة;
class Ast_عقدة_Assign;
class Ast_عقدة_DType;

//============================================================================

class V3Width final {
عام:
    ساكن فراغ width(AstNetlist* nodep) VL_MT_DISABLED;
    ساكن Ast_عقدة* widthParamsEdit(Ast_عقدة* nodep) VL_MT_DISABLED;
    ساكن Ast_عقدة* widthGenerateParamsEdit(Ast_عقدة* nodep) VL_MT_DISABLED;
    ساكن Ast_عقدة* selectNonConstantRecurse(Ast_عقدة* nodep, bool inSel = false) VL_MT_DISABLED;
    ساكن فراغ streamAssignLowerEdit(Ast_عقدة_Assign* nodep) VL_MT_DISABLED;

    // For use only in WidthVisitor
    // Replace AstSelBit, etc with AstSel/AstArraySel
    // Returns replacement node if nodep was deleted, or null if none.
    ساكن Ast_عقدة* widthSelNoIterEdit(Ast_عقدة* nodep) VL_MT_DISABLED;
};

#endif  // Guard
