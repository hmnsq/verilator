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

#నిర్వచితం_కాకపోతే VERILATOR_V3_వెడల్పు_H
#నిర్వచించు VERILATOR_V3_వెడల్పు_H

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

class AstNetlist;
class Ast_కణుపు;
class Ast_కణుపు_కేటాయించు;
class Ast_కణుపు_DType;

//============================================================================

class V3_వెడల్పు final {
బహిరంగ:
    స్థిర శూన్య వెడల్పు(AstNetlist* nodep) VL_MT_DISABLED;
    స్థిర Ast_కణుపు* వెడల్పు_Params_Edit(Ast_కణుపు* nodep) VL_MT_DISABLED;
    స్థిర Ast_కణుపు* వెడల్పు_Generate_Params_Edit(Ast_కణుపు* nodep) VL_MT_DISABLED;
    స్థిర Ast_కణుపు* selectNonConstantRecurse(Ast_కణుపు* nodep, bool లోపల_Sel = false) VL_MT_DISABLED;
    స్థిర శూన్య ప్రవాహం_కేటాయించు_క్రింది_Edit(Ast_కణుపు_కేటాయించు* nodep) VL_MT_DISABLED;

    // For use only in WidthVisitor
    // Replace AstSelBit, etc with AstSel/AstArraySel
    // Returns replacement node if nodep was deleted, or null if none.
    స్థిర Ast_కణుపు* వెడల్పు_Sel_No_Iter_Edit(Ast_కణుపు* nodep) VL_MT_DISABLED;
};

#అయితే_ముగింపు  // Guard
