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

#اگر_تعریف_نشده VERILATOR_V3WIDTHREMOVE_H_
#تعریف_کن VERILATOR_V3WIDTHREMOVE_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"

// clang-format off
#اگر_تعریف_نشده VERILATOR_V3_عرض_CPP
# error "V3WidthRemove for V3Width internal use only"
#پایان_اگر
// clang-format on

//######################################################################

class عرض_Remove_Visitor final : عام VNVisitor {
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

    فراغ replace_With_Signed_نسخه(Ast_گره* nodep, Ast_گره* newp) {
        UINFO(6, " Replace " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    // VISITORS
    فراغ بازدید(AstSigned* nodep) override {
        VL_DO_DANGLING(replace_With_Signed_نسخه(nodep, nodep->lhsp()->unlink_Fr_پشت()), nodep);
    }
    فراغ بازدید(AstUnsigned* nodep) override {
        VL_DO_DANGLING(replace_With_Signed_نسخه(nodep, nodep->lhsp()->unlink_Fr_پشت()), nodep);
    }
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    عرض_Remove_Visitor() = افتراضي;
    ~عرض_Remove_Visitor() override = افتراضي;
    Ast_گره* mainAcceptEdit(Ast_گره* nodep) { أرجع iterateSubtreeReturnEdits(nodep); }
};

//######################################################################

#پایان_اگر  // Guard
