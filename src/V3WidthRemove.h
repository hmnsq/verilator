// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Cleanup stage in V3Width
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

#నిర్వచితం_కాకపోతే VERILATOR_V3WIDTHREMOVE_H_
#నిర్వచించు VERILATOR_V3WIDTHREMOVE_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Error.h"

// clang-format off
#నిర్వచితం_కాకపోతే VERILATOR_V3_వెడల్పు_CPP
# error "V3WidthRemove for V3Width internal use only"
#అయితే_ముగింపు
// clang-format on

//######################################################################

class వెడల్పు_Remove_Visitor final : బహిరంగ VNVisitor {
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

    శూన్య replace_With_Signed_సంస్కరణ(Ast_కణుపు* nodep, Ast_కణుపు* newp) {
        UINFO(6, " Replace " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    // VISITORS
    శూన్య సందర్శించు(AstSigned* nodep) override {
        VL_DO_DANGLING(replace_With_Signed_సంస్కరణ(nodep, nodep->lhsp()->unlink_Fr_వెనుక()), nodep);
    }
    శూన్య సందర్శించు(AstUnsigned* nodep) override {
        VL_DO_DANGLING(replace_With_Signed_సంస్కరణ(nodep, nodep->lhsp()->unlink_Fr_వెనుక()), nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    వెడల్పు_Remove_Visitor() = అప్రమేయ;
    ~వెడల్పు_Remove_Visitor() override = అప్రమేయ;
    Ast_కణుపు* mainAcceptEdit(Ast_కణుపు* nodep) { తిరిగి iterateSubtreeReturnEdits(nodep); }
};

//######################################################################

#అయితే_ముగింపు  // Guard
