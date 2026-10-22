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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

class AstNetlist;
class Ast_കണ്ണി;
class Ast_കണ്ണി_Assign;
class Ast_കണ്ണി_DType;

//============================================================================

class V3Width final {
പൊതു:
    സ്ഥിതം ശൂന്യം width(AstNetlist* nodep) VL_MT_DISABLED;
    സ്ഥിതം Ast_കണ്ണി* widthParamsEdit(Ast_കണ്ണി* nodep) VL_MT_DISABLED;
    സ്ഥിതം Ast_കണ്ണി* widthGenerateParamsEdit(Ast_കണ്ണി* nodep) VL_MT_DISABLED;
    സ്ഥിതം Ast_കണ്ണി* selectNonConstantRecurse(Ast_കണ്ണി* nodep, bool inSel = false) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം streamAssignLowerEdit(Ast_കണ്ണി_Assign* nodep) VL_MT_DISABLED;

    // For use only in WidthVisitor
    // Replace AstSelBit, etc with AstSel/AstArraySel
    // Returns replacement node if nodep was deleted, or null if none.
    സ്ഥിതം Ast_കണ്ണി* widthSelNoIterEdit(Ast_കണ്ണി* nodep) VL_MT_DISABLED;
};

#endif  // Guard
