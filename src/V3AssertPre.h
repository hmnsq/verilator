// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Assertion pre-expansion
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2005-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#यदि_अपरिभाषितम् VERILATOR_V3ASSERTPRE_H_
#परिभाषय VERILATOR_V3ASSERTPRE_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

class AstNetlist;

//============================================================================

class V3AssertPre final {
सार्वजनिक:
    स्थिर शून्य assertPreAll(AstNetlist* nodep) VL_MT_DISABLED;
};

#यदि_अन्तः  // Guard
