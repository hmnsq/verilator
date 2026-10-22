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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

//============================================================================

// Forward declare so we can include this in V3Ast.h
class AstNetlist;
class Ast_गाँठो;

class V3Broken final {
सार्वजनिक:
    स्थिर शून्य brokenAll(AstNetlist* nodep);
    स्थिर bool isLinkable(अचर Ast_गाँठो* nodep);
    स्थिर शून्य addNewed(अचर Ast_गाँठो* nodep);
    स्थिर शून्य deleted(अचर Ast_गाँठो* nodep);
    // Called on error to say may be inside visitor,
    // Disables checks that may misfire if not called at stable point between visitors
    स्थिर शून्य allowMidvisitorCheck(bool flag);
    स्थिर शून्य selfTest();
};

#endif  // Guard
