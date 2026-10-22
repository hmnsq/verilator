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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3WIDTHREMOVE_H_
#وصف_ڪريو VERILATOR_V3WIDTHREMOVE_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"

// clang-format off
#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3_ويڪر_CPP
# error "V3WidthRemove for V3Width internal use only"
#جيڪڏهن_پورو
// clang-format on

//######################################################################

class ويڪر_Remove_Visitor final : عام VNVisitor {
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

    فراغ replace_With_Signed_نسخو(Ast_ڳنڍ* nodep, Ast_ڳنڍ* newp) {
        UINFO(6, " Replace " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }

    // VISITORS
    فراغ گهمو(AstSigned* nodep) override {
        VL_DO_DANGLING(replace_With_Signed_نسخو(nodep, nodep->lhsp()->unlink_Fr_پٺ()), nodep);
    }
    فراغ گهمو(AstUnsigned* nodep) override {
        VL_DO_DANGLING(replace_With_Signed_نسخو(nodep, nodep->lhsp()->unlink_Fr_پٺ()), nodep);
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    ويڪر_Remove_Visitor() = افتراضي;
    ~ويڪر_Remove_Visitor() override = افتراضي;
    Ast_ڳنڍ* mainAcceptEdit(Ast_ڳنڍ* nodep) { أرجع iterateSubtreeReturnEdits(nodep); }
};

//######################################################################

#جيڪڏهن_پورو  // Guard
