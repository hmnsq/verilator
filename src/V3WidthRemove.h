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

#ifndef VERILATOR_V3WIDTHREMOVE_H_
#define VERILATOR_V3WIDTHREMOVE_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"

// clang-format off
#ifndef VERILATOR_V3WIDTH_CPP_
# error "V3WidthRemove for V3Width internal use only"
#endif
// clang-format on

//######################################################################

class WidthRemoveVisitor final : عام VNVisitor {
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

    فراغ replaceWithSignedVersion(Ast_عقدة* nodep, Ast_عقدة* newp) {
        UINFO(6, " Replace " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }

    // VISITORS
    فراغ visit(AstSigned* nodep) override {
        VL_DO_DANGLING(replaceWithSignedVersion(nodep, nodep->lhsp()->unlinkFrBack()), nodep);
    }
    فراغ visit(AstUnsigned* nodep) override {
        VL_DO_DANGLING(replaceWithSignedVersion(nodep, nodep->lhsp()->unlinkFrBack()), nodep);
    }
    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    WidthRemoveVisitor() = افتراضي;
    ~WidthRemoveVisitor() override = افتراضي;
    Ast_عقدة* mainAcceptEdit(Ast_عقدة* nodep) { إرجاع iterateSubtreeReturnEdits(nodep); }
};

//######################################################################

#endif  // Guard
