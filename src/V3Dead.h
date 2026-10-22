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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

class AstNetlist;

//============================================================================

class V3Dead final {
सार्वजनिक:
    // Modules, no vars/dtypes
    स्थिर शून्य deadifyModules(AstNetlist* nodep) VL_MT_DISABLED;
    // Modules, Data types
    स्थिर शून्य deadifyDTypes(AstNetlist* nodep) VL_MT_DISABLED;
    स्थिर शून्य deadifyDTypesScoped(AstNetlist* nodep) VL_MT_DISABLED;
    // Everything that's possible
    स्थिर शून्य deadifyAll(AstNetlist* nodep) VL_MT_DISABLED;
    स्थिर शून्य deadifyAllScoped(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
