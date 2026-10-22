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

#अगर_अघोषित VERILATOR_V3WIDTHCOMMIT_H_
#घोषणा VERILATOR_V3WIDTHCOMMIT_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3Error.h"

//============================================================================

class V3WidthCommit final {
खुला:
    अटल AstConst* new_If_Const_Commit_आकार(AstConst* nodep) {
        अगर (((nodep->dtypep()->width() != nodep->अंक().width()) || !nodep->अंक().sized())
            && !nodep->अंक().isString()) {  // Need to force the number from unsized to sized
            V3_संख्या अंक{nodep, nodep->dtypep()->width(), nodep->अंक()};
            अंक.isSigned(nodep->isSigned());
            AstConst* स्थिर newp = new AstConst{nodep->fileline(), अंक};
            newp->dtypeFrom(nodep);
            newp->user1(true);
            वापस newp;
        }
        वापस nullptr;
    }

    // Final step... Mark all widths as equal
    अटल व्योम widthCommit(AstNetlist* nodep) VL_MT_DISABLED;
};

//######################################################################

#पूर्ण_अगर  // Guard
