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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3Error.h"

//============================================================================

class V3WidthCommit final {
সার্ৱজনিক:
    স্থির AstConst* newIfConstCommitSize(AstConst* nodep) {
        যদি (((nodep->dtypep()->width() != nodep->অঙ্ক().width()) || !nodep->অঙ্ক().sized())
            && !nodep->অঙ্ক().isString()) {  // Need to force the number from unsized to sized
            V3_সংখ্যা অঙ্ক{nodep, nodep->dtypep()->width(), nodep->অঙ্ক()};
            অঙ্ক.isSigned(nodep->isSigned());
            AstConst* ধ্রুৱ newp = new AstConst{nodep->fileline(), অঙ্ক};
            newp->dtypeFrom(nodep);
            newp->user1(true);
            ঘূৰাই newp;
        }
        ঘূৰাই nullptr;
    }

    // Final step... Mark all widths as equal
    স্থির শূন্য widthCommit(AstNetlist* nodep) VL_MT_DISABLED;
};

//######################################################################

#endif  // Guard
