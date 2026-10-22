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

#اگر_تعریف_نشده VERILATOR_V3WIDTHCOMMIT_H_
#تعریف_کن VERILATOR_V3WIDTHCOMMIT_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"

//============================================================================

class V3_عرض_Commit final {
عام:
    ساكن AstConst* new_If_Const_Commit_اندازه(AstConst* nodep) {
        إذا (((nodep->dtypep()->عرض() != nodep->رقم().عرض()) || !nodep->رقم().sized())
            && !nodep->رقم().isString()) {  // Need to force the number from unsized to sized
            V3_عدد رقم{nodep, nodep->dtypep()->عرض(), nodep->رقم()};
            رقم.isSigned(nodep->isSigned());
            AstConst* ثابت newp = new AstConst{nodep->fileline(), رقم};
            newp->dtypeFrom(nodep);
            newp->user1(true);
            أرجع newp;
        }
        أرجع nullptr;
    }

    // Final step... Mark all widths as equal
    ساكن فراغ عرض_Commit(AstNetlist* nodep) VL_MT_DISABLED;
};

//######################################################################

#پایان_اگر  // Guard
