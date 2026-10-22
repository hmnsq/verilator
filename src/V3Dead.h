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

#अगर_अघोषित VERILATOR_V3DEAD_H_
#घोषणा VERILATOR_V3DEAD_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

class AstNetlist;

//============================================================================

class V3Dead final {
खुला:
    // Modules, no vars/dtypes
    अटल व्योम deadifyModules(AstNetlist* nodep) VL_MT_DISABLED;
    // Modules, Data types
    अटल व्योम deadifyDTypes(AstNetlist* nodep) VL_MT_DISABLED;
    अटल व्योम deadifyDTypesScoped(AstNetlist* nodep) VL_MT_DISABLED;
    // Everything that's possible
    अटल व्योम deadifyAll(AstNetlist* nodep) VL_MT_DISABLED;
    अटल व्योम deadifyAllScoped(AstNetlist* nodep) VL_MT_DISABLED;
};

#पूर्ण_अगर  // Guard
