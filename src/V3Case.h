// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Break case statements up and add Unknown assigns
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

#ifndef VERILATOR_V3CASE_H_
#define VERILATOR_V3CASE_H_

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

class AstNetlist;
class AstGenCase;

//============================================================================

class V3Case final {
பொது:
    நிலையான வெறுமை caseAll(AstNetlist* nodep) VL_MT_DISABLED;
    நிலையான வெறுமை caseLint(AstGenCase* nodep) VL_MT_DISABLED;
};

#endif  // Guard
