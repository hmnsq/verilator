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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Error.h"

// clang-format off
#ifndef VERILATOR_V3WIDTH_CPP_
# error "V3WidthRemove for V3Width internal use only"
#endif
// clang-format on

//######################################################################

class WidthRemoveVisitor final : பொது VNVisitor {
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

    வெறுமை replaceWithSignedVersion(Ast_முனை* nodep, Ast_முனை* newp) {
        UINFO(6, " Replace " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    // VISITORS
    வெறுமை visit(AstSigned* nodep) override {
        VL_DO_DANGLING(replaceWithSignedVersion(nodep, nodep->lhsp()->unlinkFrBack()), nodep);
    }
    வெறுமை visit(AstUnsigned* nodep) override {
        VL_DO_DANGLING(replaceWithSignedVersion(nodep, nodep->lhsp()->unlinkFrBack()), nodep);
    }
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    WidthRemoveVisitor() = இயல்பு;
    ~WidthRemoveVisitor() override = இயல்பு;
    Ast_முனை* mainAcceptEdit(Ast_முனை* nodep) { திரும்பு iterateSubtreeReturnEdits(nodep); }
};

//######################################################################

#endif  // Guard
