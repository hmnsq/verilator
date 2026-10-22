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

#ifndef VERILATOR_V3BROKEN_H_
#define VERILATOR_V3BROKEN_H_

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

//============================================================================

// Forward declare so we can include this in V3Ast.h
class AstNetlist;
class Ast_ગાંઠ;

class V3Broken final {
જાહેર:
    સ્થિર શૂન્ય brokenAll(AstNetlist* nodep);
    સ્થિર bool isLinkable(અચલ Ast_ગાંઠ* nodep);
    સ્થિર શૂન્ય addNewed(અચલ Ast_ગાંઠ* nodep);
    સ્થિર શૂન્ય deleted(અચલ Ast_ગાંઠ* nodep);
    // Called on error to say may be inside visitor,
    // Disables checks that may misfire if not called at stable point between visitors
    સ્થિર શૂન્ય allowMidvisitorCheck(bool flag);
    સ્થિર શૂન્ય selfTest();
};

#endif  // Guard
