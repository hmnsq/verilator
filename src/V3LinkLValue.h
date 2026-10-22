// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Link modules/signals together
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

#ifndef VERILATOR_V3LINKLVALUE_H_
#define VERILATOR_V3LINKLVALUE_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"

//============================================================================

class V3LinkLValue final {
सार्वजनिक:
    स्थिर शून्य linkLValue(AstNetlist* nodep) VL_MT_DISABLED;
    स्थिर शून्य linkLValueSet(Ast_गाठ* nodep, VAccess access = VAccess::WRITE) VL_MT_DISABLED;
};

#endif  // Guard
