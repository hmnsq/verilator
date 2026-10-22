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

#اگر_غیر_تعریف_شدہ VERILATOR_V3WIDTHREMOVE_H_
#تعریف_کرو VERILATOR_V3WIDTHREMOVE_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Error.h"

// clang-format off
#اگر_غیر_تعریف_شدہ VERILATOR_V3_چوڑائی_CPP
# error "V3WidthRemove for V3Width internal use only"
#اگر_ختم
// clang-format on

//######################################################################

class چوڑائی_Remove_Visitor final : عوامی VNVisitor {
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

    خالی replace_With_Signed_نسخہ(Ast_گرہ* nodep, Ast_گرہ* newp) {
        UINFO(6, " Replace " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    // VISITORS
    خالی دورہ(AstSigned* nodep) override {
        VL_DO_DANGLING(replace_With_Signed_نسخہ(nodep, nodep->lhsp()->unlink_Fr_عقب()), nodep);
    }
    خالی دورہ(AstUnsigned* nodep) override {
        VL_DO_DANGLING(replace_With_Signed_نسخہ(nodep, nodep->lhsp()->unlink_Fr_عقب()), nodep);
    }
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    چوڑائی_Remove_Visitor() = طےشدہ;
    ~چوڑائی_Remove_Visitor() override = طےشدہ;
    Ast_گرہ* mainAcceptEdit(Ast_گرہ* nodep) { واپس iterateSubtreeReturnEdits(nodep); }
};

//######################################################################

#اگر_ختم  // Guard
