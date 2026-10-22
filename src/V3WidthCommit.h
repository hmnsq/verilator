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

#اگر_غیر_تعریف_شدہ VERILATOR_V3WIDTHCOMMIT_H_
#تعریف_کرو VERILATOR_V3WIDTHCOMMIT_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Error.h"

//============================================================================

class V3_چوڑائی_Commit final {
عوامی:
    ساکن AstConst* new_If_Const_Commit_سائز(AstConst* nodep) {
        اگر (((nodep->dtypep()->چوڑائی() != nodep->عدد().چوڑائی()) || !nodep->عدد().sized())
            && !nodep->عدد().isString()) {  // Need to force the number from unsized to sized
            V3_نمبر عدد{nodep, nodep->dtypep()->چوڑائی(), nodep->عدد()};
            عدد.isSigned(nodep->isSigned());
            AstConst* ثابت newp = new AstConst{nodep->fileline(), عدد};
            newp->dtypeFrom(nodep);
            newp->user1(true);
            واپس newp;
        }
        واپس nullptr;
    }

    // Final step... Mark all widths as equal
    ساکن خالی چوڑائی_Commit(AstNetlist* nodep) VL_MT_DISABLED;
};

//######################################################################

#اگر_ختم  // Guard
