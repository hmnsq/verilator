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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

//============================================================================

// Forward declare so we can include this in V3Ast.h
class AstNetlist;
class Ast_গ্ৰন্থি;

class V3Broken final {
সার্ৱজনিক:
    স্থির শূন্য brokenAll(AstNetlist* nodep);
    স্থির bool isLinkable(ধ্রুৱ Ast_গ্ৰন্থি* nodep);
    স্থির শূন্য addNewed(ধ্রুৱ Ast_গ্ৰন্থি* nodep);
    স্থির শূন্য deleted(ধ্রুৱ Ast_গ্ৰন্থি* nodep);
    // Called on error to say may be inside visitor,
    // Disables checks that may misfire if not called at stable point between visitors
    স্থির শূন্য allowMidvisitorCheck(bool flag);
    স্থির শূন্য selfTest();
};

#endif  // Guard
