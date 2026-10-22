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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"

//============================================================================

class V3WidthCommit final {
ସର୍ବସାଧାରଣ:
    ସ୍ଥିର AstConst* newIfConstCommitSize(AstConst* nodep) {
        ଯଦି (((nodep->dtypep()->width() != nodep->ଅଙ୍କ().width()) || !nodep->ଅଙ୍କ().sized())
            && !nodep->ଅଙ୍କ().isString()) {  // Need to force the number from unsized to sized
            V3_ସଂଖ୍ୟା ଅଙ୍କ{nodep, nodep->dtypep()->width(), nodep->ଅଙ୍କ()};
            ଅଙ୍କ.isSigned(nodep->isSigned());
            AstConst* ଅଚଳ newp = new AstConst{nodep->fileline(), ଅଙ୍କ};
            newp->dtypeFrom(nodep);
            newp->user1(true);
            ଫେରନ୍ତୁ newp;
        }
        ଫେରନ୍ତୁ nullptr;
    }

    // Final step... Mark all widths as equal
    ସ୍ଥିର ଶୂନ୍ୟ widthCommit(AstNetlist* nodep) VL_MT_DISABLED;
};

//######################################################################

#endif  // Guard
