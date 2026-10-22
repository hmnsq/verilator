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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Error.h"

//============================================================================

class V3WidthCommit final {
பொது:
    நிலையான AstConst* newIfConstCommitSize(AstConst* nodep) {
        எனில் (((nodep->dtypep()->width() != nodep->இலக்கம்().width()) || !nodep->இலக்கம்().sized())
            && !nodep->இலக்கம்().isString()) {  // Need to force the number from unsized to sized
            V3_எண் இலக்கம்{nodep, nodep->dtypep()->width(), nodep->இலக்கம்()};
            இலக்கம்.isSigned(nodep->isSigned());
            AstConst* மாறிலி newp = new AstConst{nodep->fileline(), இலக்கம்};
            newp->dtypeFrom(nodep);
            newp->user1(true);
            திரும்பு newp;
        }
        திரும்பு nullptr;
    }

    // Final step... Mark all widths as equal
    நிலையான வெறுமை widthCommit(AstNetlist* nodep) VL_MT_DISABLED;
};

//######################################################################

#endif  // Guard
