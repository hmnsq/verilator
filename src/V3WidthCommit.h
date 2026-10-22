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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3WIDTHCOMMIT_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3WIDTHCOMMIT_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"

//============================================================================

class V3_ಅಗಲ_Commit final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ AstConst* new_If_Const_Commit_ಗಾತ್ರ(AstConst* nodep) {
        ಆದರೆ (((nodep->dtypep()->ಅಗಲ() != nodep->ಅಂಕೆ().ಅಗಲ()) || !nodep->ಅಂಕೆ().sized())
            && !nodep->ಅಂಕೆ().isString()) {  // Need to force the number from unsized to sized
            V3_ಸಂಖ್ಯೆ ಅಂಕೆ{nodep, nodep->dtypep()->ಅಗಲ(), nodep->ಅಂಕೆ()};
            ಅಂಕೆ.isSigned(nodep->isSigned());
            AstConst* ಸ್ಥಿರಾಂಕ newp = new AstConst{nodep->fileline(), ಅಂಕೆ};
            newp->dtypeFrom(nodep);
            newp->user1(true);
            ಹಿಂತಿರುಗಿ newp;
        }
        ಹಿಂತಿರುಗಿ nullptr;
    }

    // Final step... Mark all widths as equal
    ಸ್ಥಿರ ಶೂನ್ಯ ಅಗಲ_Commit(AstNetlist* nodep) VL_MT_DISABLED;
};

//######################################################################

#ಆದರೆ_ಅಂತ್ಯ  // Guard
