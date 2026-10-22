// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Break always into sensitivity inst domains
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

#ifndef VERILATOR_V3INST_H_
#define VERILATOR_V3INST_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

class AstAssignW;
class AstCell;
class AstNetlist;
class AstPin;

//============================================================================

class V3Inst final {
सार्वजनिक:
    स्थिर शून्य instAll(AstNetlist* nodep) VL_MT_DISABLED;
    स्थिर AstAssignW* pinReconnectSimple(AstPin* pinp, AstCell* cellp, bool forTristate,
                                          bool alwaysCvt = false) VL_MT_DISABLED;
    स्थिर शून्य checkOutputShort(स्थिरांक AstPin* nodep) VL_MT_DISABLED;
};

#endif  // Guard
