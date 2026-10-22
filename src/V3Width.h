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

#अगर_अघोषित VERILATOR_V3WIDTH_H_
#घोषणा VERILATOR_V3WIDTH_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

class AstNetlist;
class Ast_गाँठ;
class Ast_गाँठ_Assign;
class Ast_गाँठ_DType;

//============================================================================

class V3Width final {
खुला:
    अटल व्योम width(AstNetlist* nodep) VL_MT_DISABLED;
    अटल Ast_गाँठ* widthParamsEdit(Ast_गाँठ* nodep) VL_MT_DISABLED;
    अटल Ast_गाँठ* widthGenerateParamsEdit(Ast_गाँठ* nodep) VL_MT_DISABLED;
    अटल Ast_गाँठ* selectNonConstantRecurse(Ast_गाँठ* nodep, bool inSel = false) VL_MT_DISABLED;
    अटल व्योम streamAssignLowerEdit(Ast_गाँठ_Assign* nodep) VL_MT_DISABLED;

    // For use only in WidthVisitor
    // Replace AstSelBit, etc with AstSel/AstArraySel
    // Returns replacement node if nodep was deleted, or null if none.
    अटल Ast_गाँठ* widthSelNoIterEdit(Ast_गाँठ* nodep) VL_MT_DISABLED;
};

#पूर्ण_अगर  // Guard
