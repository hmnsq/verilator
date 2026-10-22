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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"

// clang-format off
#ifndef VERILATOR_V3WIDTH_CPP_
# error "V3WidthRemove for V3Width internal use only"
#endif
// clang-format on

//######################################################################

class WidthRemoveVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

    ಶೂನ್ಯ replaceWithSignedVersion(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* newp) {
        UINFO(6, " Replace " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstSigned* nodep) override {
        VL_DO_DANGLING(replaceWithSignedVersion(nodep, nodep->lhsp()->unlinkFrBack()), nodep);
    }
    ಶೂನ್ಯ visit(AstUnsigned* nodep) override {
        VL_DO_DANGLING(replaceWithSignedVersion(nodep, nodep->lhsp()->unlinkFrBack()), nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    WidthRemoveVisitor() = ಸ್ವಭಾವ;
    ~WidthRemoveVisitor() override = ಸ್ವಭಾವ;
    Ast_ಗಂಟು* mainAcceptEdit(Ast_ಗಂಟು* nodep) { ಪ್ರತ್ಯಾಗಮ iterateSubtreeReturnEdits(nodep); }
};

//######################################################################

#endif  // Guard
