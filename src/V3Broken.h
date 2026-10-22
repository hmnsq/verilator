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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

//============================================================================

// Forward declare so we can include this in V3Ast.h
class AstNetlist;
class Ast_കണ്ണി;

class V3Broken final {
പൊതു:
    സ്ഥിതം ശൂന്യം brokenAll(AstNetlist* nodep);
    സ്ഥിതം bool isLinkable(സ്ഥിരം Ast_കണ്ണി* nodep);
    സ്ഥിതം ശൂന്യം addNewed(സ്ഥിരം Ast_കണ്ണി* nodep);
    സ്ഥിതം ശൂന്യം deleted(സ്ഥിരം Ast_കണ്ണി* nodep);
    // Called on error to say may be inside visitor,
    // Disables checks that may misfire if not called at stable point between visitors
    സ്ഥിതം ശൂന്യം allowMidvisitorCheck(bool flag);
    സ്ഥിതം ശൂന്യം selfTest();
};

#endif  // Guard
