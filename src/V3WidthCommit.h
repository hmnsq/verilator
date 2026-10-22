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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3WIDTHCOMMIT_H_
#وصف_ڪريو VERILATOR_V3WIDTHCOMMIT_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"

//============================================================================

class V3_ويڪر_Commit final {
عام:
    ساكن AstConst* new_If_Const_Commit_سائيز(AstConst* nodep) {
        إذا (((nodep->dtypep()->ويڪر() != nodep->انگ().ويڪر()) || !nodep->انگ().sized())
            && !nodep->انگ().isString()) {  // Need to force the number from unsized to sized
            V3_عدد انگ{nodep, nodep->dtypep()->ويڪر(), nodep->انگ()};
            انگ.isSigned(nodep->isSigned());
            AstConst* ثابت newp = new AstConst{nodep->fileline(), انگ};
            newp->dtypeFrom(nodep);
            newp->user1(true);
            أرجع newp;
        }
        أرجع nullptr;
    }

    // Final step... Mark all widths as equal
    ساكن فراغ ويڪر_Commit(AstNetlist* nodep) VL_MT_DISABLED;
};

//######################################################################

#جيڪڏهن_پورو  // Guard
