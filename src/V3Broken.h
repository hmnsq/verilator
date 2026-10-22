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

#اگر_غیر_تعریف_شدہ VERILATOR_V3BROKEN_H_
#تعریف_کرو VERILATOR_V3BROKEN_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

//============================================================================

// Forward declare so we can include this in V3Ast.h
class AstNetlist;
class Ast_گرہ;

class V3Broken final {
عوامی:
    ساکن خالی brokenAll(AstNetlist* nodep);
    ساکن bool isLinkable(ثابت Ast_گرہ* nodep);
    ساکن خالی جوڑو_Newed(ثابت Ast_گرہ* nodep);
    ساکن خالی deleted(ثابت Ast_گرہ* nodep);
    // Called on error to say may be inside visitor,
    // Disables checks that may misfire if not called at stable point between visitors
    ساکن خالی allowMidvisitorCheck(bool flag);
    ساکن خالی selfTest();
};

#اگر_ختم  // Guard
