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

#جے_بنا_تعریف VERILATOR_V3WIDTHREMOVE_H_
#تعریف_کرو VERILATOR_V3WIDTHREMOVE_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"

// clang-format off
#جے_بنا_تعریف VERILATOR_V3_چوڑائی_CPP
# error "V3WidthRemove for V3Width internal use only"
#جے_انت
// clang-format on

//######################################################################

class چوڑائی_Remove_Visitor final : عام VNVisitor {
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

    فراغ replace_With_Signed_نسخہ(Ast_گرہ* nodep, Ast_گرہ* newp) {
        UINFO(6, " Replace " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    // VISITORS
    فراغ ویکھو(AstSigned* nodep) override {
        VL_DO_DANGLING(replace_With_Signed_نسخہ(nodep, nodep->lhsp()->unlink_Fr_پٹھ()), nodep);
    }
    فراغ ویکھو(AstUnsigned* nodep) override {
        VL_DO_DANGLING(replace_With_Signed_نسخہ(nodep, nodep->lhsp()->unlink_Fr_پٹھ()), nodep);
    }
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    چوڑائی_Remove_Visitor() = افتراضي;
    ~چوڑائی_Remove_Visitor() override = افتراضي;
    Ast_گرہ* mainAcceptEdit(Ast_گرہ* nodep) { أرجع iterateSubtreeReturnEdits(nodep); }
};

//######################################################################

#جے_انت  // Guard
