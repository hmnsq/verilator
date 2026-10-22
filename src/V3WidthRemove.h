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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3WIDTHREMOVE_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3WIDTHREMOVE_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"

// clang-format off
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3_ಅಗಲ_CPP
# error "V3WidthRemove for V3Width internal use only"
#ಆದರೆ_ಅಂತ್ಯ
// clang-format on

//######################################################################

class ಅಗಲ_Remove_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

    ಶೂನ್ಯ replace_With_Signed_ಆವೃತ್ತಿ(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* newp) {
        UINFO(6, " Replace " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstSigned* nodep) override {
        VL_DO_DANGLING(replace_With_Signed_ಆವೃತ್ತಿ(nodep, nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstUnsigned* nodep) override {
        VL_DO_DANGLING(replace_With_Signed_ಆವೃತ್ತಿ(nodep, nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಅಗಲ_Remove_Visitor() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~ಅಗಲ_Remove_Visitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
    Ast_ಗಂಟು* mainAcceptEdit(Ast_ಗಂಟು* nodep) { ಹಿಂತಿರುಗಿ iterateSubtreeReturnEdits(nodep); }
};

//######################################################################

#ಆದರೆ_ಅಂತ್ಯ  // Guard
