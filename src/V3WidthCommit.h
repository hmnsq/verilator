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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Error.h"

//============================================================================

class V3WidthCommit final {
عمومی:
    ایستا AstConst* newIfConstCommitSize(AstConst* nodep) {
        اگر (((nodep->dtypep()->width() != nodep->رقم().width()) || !nodep->رقم().sized())
            && !nodep->رقم().isString()) {  // Need to force the number from unsized to sized
            V3_عدد رقم{nodep, nodep->dtypep()->width(), nodep->رقم()};
            رقم.isSigned(nodep->isSigned());
            AstConst* ثابت newp = new AstConst{nodep->fileline(), رقم};
            newp->dtypeFrom(nodep);
            newp->user1(true);
            بازگشت newp;
        }
        بازگشت nullptr;
    }

    // Final step... Mark all widths as equal
    ایستا تهی widthCommit(AstNetlist* nodep) VL_MT_DISABLED;
};

//######################################################################

#endif  // Guard
