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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"

// clang-format off
#ifndef VERILATOR_V3WIDTH_CPP_
# error "V3WidthRemove for V3Width internal use only"
#endif
// clang-format on

//######################################################################

class WidthRemoveVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

    ଶୂନ୍ୟ replaceWithSignedVersion(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି* newp) {
        UINFO(6, " Replace " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstSigned* nodep) override {
        VL_DO_DANGLING(replaceWithSignedVersion(nodep, nodep->lhsp()->unlinkFrBack()), nodep);
    }
    ଶୂନ୍ୟ visit(AstUnsigned* nodep) override {
        VL_DO_DANGLING(replaceWithSignedVersion(nodep, nodep->lhsp()->unlinkFrBack()), nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    WidthRemoveVisitor() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~WidthRemoveVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    Ast_ଗଣ୍ଠି* mainAcceptEdit(Ast_ଗଣ୍ଠି* nodep) { ଫେରନ୍ତୁ iterateSubtreeReturnEdits(nodep); }
};

//######################################################################

#endif  // Guard
