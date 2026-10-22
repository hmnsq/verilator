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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

class AstNetlist;
class Ast_ଗଣ୍ଠି;
class Ast_ଗଣ୍ଠି_Assign;
class Ast_ଗଣ୍ଠି_DType;

//============================================================================

class V3Width final {
ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ width(AstNetlist* nodep) VL_MT_DISABLED;
    ସ୍ଥିର Ast_ଗଣ୍ଠି* widthParamsEdit(Ast_ଗଣ୍ଠି* nodep) VL_MT_DISABLED;
    ସ୍ଥିର Ast_ଗଣ୍ଠି* widthGenerateParamsEdit(Ast_ଗଣ୍ଠି* nodep) VL_MT_DISABLED;
    ସ୍ଥିର Ast_ଗଣ୍ଠି* selectNonConstantRecurse(Ast_ଗଣ୍ଠି* nodep, bool inSel = false) VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ streamAssignLowerEdit(Ast_ଗଣ୍ଠି_Assign* nodep) VL_MT_DISABLED;

    // For use only in WidthVisitor
    // Replace AstSelBit, etc with AstSel/AstArraySel
    // Returns replacement node if nodep was deleted, or null if none.
    ସ୍ଥିର Ast_ଗଣ୍ଠି* widthSelNoIterEdit(Ast_ଗଣ୍ଠି* nodep) VL_MT_DISABLED;
};

#endif  // Guard
