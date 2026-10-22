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

#अगर_अघोषित VERILATOR_V3BROKEN_H_
#घोषणा VERILATOR_V3BROKEN_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

//============================================================================

// Forward declare so we can include this in V3Ast.h
class AstNetlist;
class Ast_गाँठ;

class V3Broken final {
खुला:
    अटल व्योम brokenAll(AstNetlist* nodep);
    अटल bool isLinkable(स्थिर Ast_गाँठ* nodep);
    अटल व्योम addNewed(स्थिर Ast_गाँठ* nodep);
    अटल व्योम deleted(स्थिर Ast_गाँठ* nodep);
    // Called on error to say may be inside visitor,
    // Disables checks that may misfire if not called at stable point between visitors
    अटल व्योम allowMidvisitorCheck(bool flag);
    अटल व्योम selfTest();
};

#पूर्ण_अगर  // Guard
