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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Error.h"

//============================================================================

class V3WidthCommit final {
सार्वजनिक:
    स्थिर AstConst* newIfConstCommitSize(AstConst* nodep) {
        यदि (((nodep->dtypep()->width() != nodep->अङ्क().width()) || !nodep->अङ्क().sized())
            && !nodep->अङ्क().isString()) {  // Need to force the number from unsized to sized
            V3_सङ्ख्या अङ्क{nodep, nodep->dtypep()->width(), nodep->अङ्क()};
            अङ्क.isSigned(nodep->isSigned());
            AstConst* अचर newp = new AstConst{nodep->fileline(), अङ्क};
            newp->dtypeFrom(nodep);
            newp->user1(true);
            फर्काऊ newp;
        }
        फर्काऊ nullptr;
    }

    // Final step... Mark all widths as equal
    स्थिर शून्य widthCommit(AstNetlist* nodep) VL_MT_DISABLED;
};

//######################################################################

#endif  // Guard
