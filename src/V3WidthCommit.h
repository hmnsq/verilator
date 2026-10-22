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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3WIDTHCOMMIT_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3WIDTHCOMMIT_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"

//============================================================================

class V3_ਚੌੜਾਈ_Commit final {
ਜਨਤਕ:
    ਸਥਿਰ AstConst* new_If_Const_Commit_ਮਾਪ(AstConst* nodep) {
        ਜੇ (((nodep->dtypep()->ਚੌੜਾਈ() != nodep->ਅੰਕ().ਚੌੜਾਈ()) || !nodep->ਅੰਕ().sized())
            && !nodep->ਅੰਕ().isString()) {  // Need to force the number from unsized to sized
            V3_ਸੰਖਿਆ ਅੰਕ{nodep, nodep->dtypep()->ਚੌੜਾਈ(), nodep->ਅੰਕ()};
            ਅੰਕ.isSigned(nodep->isSigned());
            AstConst* ਅਚਲ newp = new AstConst{nodep->fileline(), ਅੰਕ};
            newp->dtypeFrom(nodep);
            newp->user1(true);
            ਵਾਪਸ newp;
        }
        ਵਾਪਸ nullptr;
    }

    // Final step... Mark all widths as equal
    ਸਥਿਰ ਖਾਲੀ ਚੌੜਾਈ_Commit(AstNetlist* nodep) VL_MT_DISABLED;
};

//######################################################################

#ਜੇ_ਅੰਤ  // Guard
