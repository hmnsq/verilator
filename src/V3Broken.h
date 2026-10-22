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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3BROKEN_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3BROKEN_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

//============================================================================

// Forward declare so we can include this in V3Ast.h
class AstNetlist;
class Ast_ਗੰਢ;

class V3Broken final {
ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ brokenAll(AstNetlist* nodep);
    ਸਥਿਰ bool isLinkable(ਅਚਲ Ast_ਗੰਢ* nodep);
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Newed(ਅਚਲ Ast_ਗੰਢ* nodep);
    ਸਥਿਰ ਖਾਲੀ deleted(ਅਚਲ Ast_ਗੰਢ* nodep);
    // Called on error to say may be inside visitor,
    // Disables checks that may misfire if not called at stable point between visitors
    ਸਥਿਰ ਖਾਲੀ allowMidvisitorCheck(bool flag);
    ਸਥਿਰ ਖਾਲੀ selfTest();
};

#ਜੇ_ਅੰਤ  // Guard
