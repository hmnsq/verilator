// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Node attributes/ expression widths
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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3_ಅಗಲ_H
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3_ಅಗಲ_H

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

class AstNetlist;
class Ast_ಗಂಟು;
class Ast_ಗಂಟು_ನಿಯೋಜಿಸು;
class Ast_ಗಂಟು_DType;

//============================================================================

class V3_ಅಗಲ final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ ಅಗಲ(AstNetlist* nodep) VL_MT_DISABLED;
    ಸ್ಥಿರ Ast_ಗಂಟು* ಅಗಲ_Params_Edit(Ast_ಗಂಟು* nodep) VL_MT_DISABLED;
    ಸ್ಥಿರ Ast_ಗಂಟು* ಅಗಲ_Generate_Params_Edit(Ast_ಗಂಟು* nodep) VL_MT_DISABLED;
    ಸ್ಥಿರ Ast_ಗಂಟು* selectNonConstantRecurse(Ast_ಗಂಟು* nodep, bool ಒಳ_Sel = false) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ ಹರಿವು_ನಿಯೋಜಿಸು_ಅಧೋ_Edit(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) VL_MT_DISABLED;

    // For use only in WidthVisitor
    // Replace AstSelBit, etc with AstSel/AstArraySel
    // Returns replacement node if nodep was deleted, or null if none.
    ಸ್ಥಿರ Ast_ಗಂಟು* ಅಗಲ_Sel_No_Iter_Edit(Ast_ಗಂಟು* nodep) VL_MT_DISABLED;
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
