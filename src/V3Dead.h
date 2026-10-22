// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Dead branch elimination
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

#ifndef VERILATOR_V3DEAD_H_
#define VERILATOR_V3DEAD_H_

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

class AstNetlist;

//============================================================================

class V3Dead final {
જાહેર:
    // Modules, no vars/dtypes
    સ્થિર શૂન્ય deadifyModules(AstNetlist* nodep) VL_MT_DISABLED;
    // Modules, Data types
    સ્થિર શૂન્ય deadifyDTypes(AstNetlist* nodep) VL_MT_DISABLED;
    સ્થિર શૂન્ય deadifyDTypesScoped(AstNetlist* nodep) VL_MT_DISABLED;
    // Everything that's possible
    સ્થિર શૂન્ય deadifyAll(AstNetlist* nodep) VL_MT_DISABLED;
    સ્થિર શૂન્ય deadifyAllScoped(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
