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

#که_نه_تعریف_شوی VERILATOR_V3_پلنوالی_H
#تعریف_کړه VERILATOR_V3_پلنوالی_H

#ضمن "config_build.h"
#ضمن "verilatedos.h"

class AstNetlist;
class Ast_غوټه;
class Ast_غوټه_ټاکل;
class Ast_غوټه_DType;

//============================================================================

class V3_پلنوالی final {
عام:
    ساكن فراغ پلنوالی(AstNetlist* nodep) VL_MT_DISABLED;
    ساكن Ast_غوټه* پلنوالی_Params_Edit(Ast_غوټه* nodep) VL_MT_DISABLED;
    ساكن Ast_غوټه* پلنوالی_Generate_Params_Edit(Ast_غوټه* nodep) VL_MT_DISABLED;
    ساكن Ast_غوټه* selectNonConstantRecurse(Ast_غوټه* nodep, bool دننه_Sel = false) VL_MT_DISABLED;
    ساكن فراغ بهیر_ټاکل_ښکتنی_Edit(Ast_غوټه_ټاکل* nodep) VL_MT_DISABLED;

    // For use only in WidthVisitor
    // Replace AstSelBit, etc with AstSel/AstArraySel
    // Returns replacement node if nodep was deleted, or null if none.
    ساكن Ast_غوټه* پلنوالی_Sel_No_Iter_Edit(Ast_غوټه* nodep) VL_MT_DISABLED;
};

#د_که_پای  // Guard
