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

#यदि_अपरिभाषितम् VERILATOR_V3_विस्तारः_H
#परिभाषय VERILATOR_V3_विस्तारः_H

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

class AstNetlist;
class Ast_ग्रन्थिः;
class Ast_ग्रन्थिः_नियोजय;
class Ast_ग्रन्थिः_DType;

//============================================================================

class V3_विस्तारः final {
सार्वजनिक:
    स्थिर शून्य विस्तारः(AstNetlist* nodep) VL_MT_DISABLED;
    स्थिर Ast_ग्रन्थिः* विस्तारः_Params_Edit(Ast_ग्रन्थिः* nodep) VL_MT_DISABLED;
    स्थिर Ast_ग्रन्थिः* विस्तारः_Generate_Params_Edit(Ast_ग्रन्थिः* nodep) VL_MT_DISABLED;
    स्थिर Ast_ग्रन्थिः* selectNonConstantRecurse(Ast_ग्रन्थिः* nodep, bool अन्तर्_Sel = false) VL_MT_DISABLED;
    स्थिर शून्य प्रवाहः_नियोजय_अधस्तनम्_Edit(Ast_ग्रन्थिः_नियोजय* nodep) VL_MT_DISABLED;

    // For use only in WidthVisitor
    // Replace AstSelBit, etc with AstSel/AstArraySel
    // Returns replacement node if nodep was deleted, or null if none.
    स्थिर Ast_ग्रन्थिः* विस्तारः_Sel_No_Iter_Edit(Ast_ग्रन्थिः* nodep) VL_MT_DISABLED;
};

#यदि_अन्तः  // Guard
