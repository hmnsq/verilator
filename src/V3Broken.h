// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Find broken links in tree
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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3BROKEN_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3BROKEN_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

//============================================================================

// Forward declare so we can include this in V3Ast.h
class AstNetlist;
class Ast_ಗಂಟು;

class V3Broken final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ brokenAll(AstNetlist* nodep);
    ಸ್ಥಿರ bool isLinkable(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep);
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Newed(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep);
    ಸ್ಥಿರ ಶೂನ್ಯ deleted(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep);
    // Called on error to say may be inside visitor,
    // Disables checks that may misfire if not called at stable point between visitors
    ಸ್ಥಿರ ಶೂನ್ಯ allowMidvisitorCheck(bool flag);
    ಸ್ಥಿರ ಶೂನ್ಯ selfTest();
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
