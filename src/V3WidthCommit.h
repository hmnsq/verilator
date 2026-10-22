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

#include "config_build.h"
#include "verilatedos.h"

#include "V3Ast.h"
#include "V3Error.h"

//============================================================================

class V3WidthCommit final {
public:
    static AstConst* newIfConstCommitSize(AstConst* nodep) {
        if (((nodep->dtypep()->width() != nodep->नुमेरो().width()) || !nodep->नुमेरो().sized())
            && !nodep->नुमेरो().isString()) {  // Need to force the number from unsized to sized
            V3_नुमेरो नुमेरो{nodep, nodep->dtypep()->width(), nodep->नुमेरो()};
            नुमेरो.isSigned(nodep->isSigned());
            AstConst* const newp = new AstConst{nodep->fileline(), नुमेरो};
            newp->dtypeFrom(nodep);
            newp->user1(true);
            return newp;
        }
        return nullptr;
    }

    // Final step... Mark all widths as equal
    static void widthCommit(AstNetlist* nodep) VL_MT_DISABLED;
};

//######################################################################

#endif  // Guard
