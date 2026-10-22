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

#నిర్వచితం_కాకపోతే VERILATOR_V3WIDTHCOMMIT_H_
#నిర్వచించు VERILATOR_V3WIDTHCOMMIT_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Error.h"

//============================================================================

class V3_వెడల్పు_Commit final {
బహిరంగ:
    స్థిర AstConst* new_If_Const_Commit_కొలత(AstConst* nodep) {
        ఒకవేళ (((nodep->dtypep()->వెడల్పు() != nodep->అంకె().వెడల్పు()) || !nodep->అంకె().sized())
            && !nodep->అంకె().isString()) {  // Need to force the number from unsized to sized
            V3_సంఖ్య అంకె{nodep, nodep->dtypep()->వెడల్పు(), nodep->అంకె()};
            అంకె.isSigned(nodep->isSigned());
            AstConst* స్థిరరాశి newp = new AstConst{nodep->fileline(), అంకె};
            newp->dtypeFrom(nodep);
            newp->user1(true);
            తిరిగి newp;
        }
        తిరిగి nullptr;
    }

    // Final step... Mark all widths as equal
    స్థిర శూన్య వెడల్పు_Commit(AstNetlist* nodep) VL_MT_DISABLED;
};

//######################################################################

#అయితే_ముగింపు  // Guard
