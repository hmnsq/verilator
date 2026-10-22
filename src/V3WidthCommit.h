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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Error.h"

//============================================================================

class V3WidthCommit final {
പൊതു:
    സ്ഥിതം AstConst* newIfConstCommitSize(AstConst* nodep) {
        എങ്കിൽ (((nodep->dtypep()->width() != nodep->അക്കം().width()) || !nodep->അക്കം().sized())
            && !nodep->അക്കം().isString()) {  // Need to force the number from unsized to sized
            V3_സംഖ്യ അക്കം{nodep, nodep->dtypep()->width(), nodep->അക്കം()};
            അക്കം.isSigned(nodep->isSigned());
            AstConst* സ്ഥിരം newp = new AstConst{nodep->fileline(), അക്കം};
            newp->dtypeFrom(nodep);
            newp->user1(true);
            തിരികെ newp;
        }
        തിരികെ nullptr;
    }

    // Final step... Mark all widths as equal
    സ്ഥിതം ശൂന്യം widthCommit(AstNetlist* nodep) VL_MT_DISABLED;
};

//######################################################################

#endif  // Guard
