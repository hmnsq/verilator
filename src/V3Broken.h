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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

//============================================================================

// Forward declare so we can include this in V3Ast.h
class AstNetlist;
class Ast_تۈگۈن;

class V3Broken final {
عام:
    ساكن فراغ brokenAll(AstNetlist* nodep);
    ساكن bool isLinkable(ثابت Ast_تۈگۈن* nodep);
    ساكن فراغ addNewed(ثابت Ast_تۈگۈن* nodep);
    ساكن فراغ deleted(ثابت Ast_تۈگۈن* nodep);
    // Called on error to say may be inside visitor,
    // Disables checks that may misfire if not called at stable point between visitors
    ساكن فراغ allowMidvisitorCheck(bool flag);
    ساكن فراغ selfTest();
};

#endif  // Guard
