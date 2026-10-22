// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Propagate constants across AST
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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3CONST_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3CONST_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"

//============================================================================

class V3Const final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ constifyParamsEdit(Ast_ಗಂಟು* nodep) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ constifyParamsNoWarnEdit(Ast_ಗಂಟು* nodep) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ constifyGenerateParamsEdit(Ast_ಗಂಟು* nodep) VL_MT_DISABLED;
    // Only do constant pushing, without removing dead logic
    ಸ್ಥಿರ ಶೂನ್ಯ constify_All_ಜೀವಂತ(AstNetlist* nodep) VL_MT_DISABLED;
    // Everything that's possible
    ಸ್ಥಿರ ಶೂನ್ಯ constifyAll(AstNetlist* nodep) VL_MT_DISABLED;
    // Also, warn
    ಸ್ಥಿರ ಶೂನ್ಯ constifyAllLint(AstNetlist* nodep) VL_MT_DISABLED;
    // C++ datatypes
    ಸ್ಥಿರ ಶೂನ್ಯ constifyCpp(AstNetlist* nodep) VL_MT_DISABLED;
    // Only the current node and lower
    // Return new node that may have replaced nodep
    ಸ್ಥಿರ Ast_ಗಂಟು* constifyEditCpp(Ast_ಗಂಟು* nodep) VL_MT_DISABLED;
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* constifyEditCpp(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) VL_MT_DISABLED {
        ಹಿಂತಿರುಗಿ VN_AS(constifyEditCpp(static_cast<Ast_ಗಂಟು*>(exprp)), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
    }
    // Only the current node and lower
    // Return new node that may have replaced nodep
    ಸ್ಥಿರ Ast_ಗಂಟು* constifyEdit(Ast_ಗಂಟು* nodep) VL_MT_DISABLED;
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* constifyEdit(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) VL_MT_DISABLED {
        ಹಿಂತಿರುಗಿ VN_AS(constifyEdit(static_cast<Ast_ಗಂಟು*>(exprp)), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
    }
    // Only the current node and lower, with special SenTree optimization
    // Return new node that may have replaced nodep
    ಸ್ಥಿರ Ast_ಗಂಟು* constifyExpensiveEdit(Ast_ಗಂಟು* nodep) VL_MT_DISABLED;
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
