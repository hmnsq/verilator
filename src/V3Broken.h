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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

//============================================================================

// Forward declare so we can include this in V3Ast.h
class AstNetlist;
class Ast_முனை;

class V3Broken final {
பொது:
    நிலையான வெறுமை brokenAll(AstNetlist* nodep);
    நிலையான bool isLinkable(மாறிலி Ast_முனை* nodep);
    நிலையான வெறுமை addNewed(மாறிலி Ast_முனை* nodep);
    நிலையான வெறுமை deleted(மாறிலி Ast_முனை* nodep);
    // Called on error to say may be inside visitor,
    // Disables checks that may misfire if not called at stable point between visitors
    நிலையான வெறுமை allowMidvisitorCheck(bool flag);
    நிலையான வெறுமை selfTest();
};

#endif  // Guard
