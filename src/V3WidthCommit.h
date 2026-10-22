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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"

//============================================================================

class V3WidthCommit final {
عام:
    ساكن AstConst* newIfConstCommitSize(AstConst* nodep) {
        ܐܢ (((nodep->dtypep()->width() != nodep->num().width()) || !nodep->num().sized())
            && !nodep->num().isString()) {  // Need to force the number from unsized to sized
            V3Number num{nodep, nodep->dtypep()->width(), nodep->num()};
            num.isSigned(nodep->isSigned());
            AstConst* ثابت newp = new AstConst{nodep->fileline(), num};
            newp->dtypeFrom(nodep);
            newp->user1(true);
            ܦܢܐ newp;
        }
        ܦܢܐ nullptr;
    }

    // Final step... Mark all widths as equal
    ساكن فراغ widthCommit(AstNetlist* nodep) VL_MT_DISABLED;
};

//######################################################################

#endif  // Guard
