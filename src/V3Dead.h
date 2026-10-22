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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

class AstNetlist;

//============================================================================

class V3Dead final {
সার্বজনিক:
    // Modules, no vars/dtypes
    স্থির শূন্য deadifyModules(AstNetlist* nodep) VL_MT_DISABLED;
    // Modules, Data types
    স্থির শূন্য deadifyDTypes(AstNetlist* nodep) VL_MT_DISABLED;
    স্থির শূন্য deadifyDTypesScoped(AstNetlist* nodep) VL_MT_DISABLED;
    // Everything that's possible
    স্থির শূন্য deadifyAll(AstNetlist* nodep) VL_MT_DISABLED;
    স্থির শূন্য deadifyAllScoped(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
