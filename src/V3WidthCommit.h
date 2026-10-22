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

#ifndef VERILATOR_V3WIDTHCOMMIT_H_
#define VERILATOR_V3WIDTHCOMMIT_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"

//============================================================================

class V3WidthCommit final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ AstConst* newIfConstCommitSize(AstConst* nodep) {
        ಯದಿ (((nodep->dtypep()->width() != nodep->ಅಂಕೆ().width()) || !nodep->ಅಂಕೆ().sized())
            && !nodep->ಅಂಕೆ().isString()) {  // Need to force the number from unsized to sized
            V3_ಸಂಖ್ಯೆ ಅಂಕೆ{nodep, nodep->dtypep()->width(), nodep->ಅಂಕೆ()};
            ಅಂಕೆ.isSigned(nodep->isSigned());
            AstConst* ಧ್ರುವ newp = new AstConst{nodep->fileline(), ಅಂಕೆ};
            newp->dtypeFrom(nodep);
            newp->user1(true);
            ಪ್ರತ್ಯಾಗಮ newp;
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    // Final step... Mark all widths as equal
    ಸ್ಥಿರ ಶೂನ್ಯ widthCommit(AstNetlist* nodep) VL_MT_DISABLED;
};

//######################################################################

#endif  // Guard
