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

#شامل "config_build.h"
#شامل "verilatedos.h"

//============================================================================

// Forward declare so we can include this in V3Ast.h
class AstNetlist;
class Ast_گره;

class V3Broken final {
عمومی:
    ایستا تهی brokenAll(AstNetlist* nodep);
    ایستا bool isLinkable(ثابت Ast_گره* nodep);
    ایستا تهی addNewed(ثابت Ast_گره* nodep);
    ایستا تهی deleted(ثابت Ast_گره* nodep);
    // Called on error to say may be inside visitor,
    // Disables checks that may misfire if not called at stable point between visitors
    ایستا تهی allowMidvisitorCheck(bool flag);
    ایستا تهی selfTest();
};

#endif  // Guard
