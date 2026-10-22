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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Error.h"

//============================================================================

class V3WidthCommit final {
જાહેર:
    સ્થિર AstConst* newIfConstCommitSize(AstConst* nodep) {
        જો (((nodep->dtypep()->width() != nodep->અંક().width()) || !nodep->અંક().sized())
            && !nodep->અંક().isString()) {  // Need to force the number from unsized to sized
            V3_સંખ્યા અંક{nodep, nodep->dtypep()->width(), nodep->અંક()};
            અંક.isSigned(nodep->isSigned());
            AstConst* અચલ newp = new AstConst{nodep->fileline(), અંક};
            newp->dtypeFrom(nodep);
            newp->user1(true);
            પાછા newp;
        }
        પાછા nullptr;
    }

    // Final step... Mark all widths as equal
    સ્થિર શૂન્ય widthCommit(AstNetlist* nodep) VL_MT_DISABLED;
};

//######################################################################

#endif  // Guard
