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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Error.h"

// clang-format off
#ifndef VERILATOR_V3WIDTH_CPP_
# error "V3WidthRemove for V3Width internal use only"
#endif
// clang-format on

//######################################################################

class WidthRemoveVisitor final : عمومی VNVisitor {
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

    تهی replaceWithSignedVersion(Ast_گره* nodep, Ast_گره* newp) {
        UINFO(6, " Replace " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    // VISITORS
    تهی visit(AstSigned* nodep) override {
        VL_DO_DANGLING(replaceWithSignedVersion(nodep, nodep->lhsp()->unlinkFrBack()), nodep);
    }
    تهی visit(AstUnsigned* nodep) override {
        VL_DO_DANGLING(replaceWithSignedVersion(nodep, nodep->lhsp()->unlinkFrBack()), nodep);
    }
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    WidthRemoveVisitor() = پیشفرض;
    ~WidthRemoveVisitor() override = پیشفرض;
    Ast_گره* mainAcceptEdit(Ast_گره* nodep) { بازگشت iterateSubtreeReturnEdits(nodep); }
};

//######################################################################

#endif  // Guard
