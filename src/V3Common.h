// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Add common contents to modules
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

#ifndef VERILATOR_V3COMMON_H_
#define VERILATOR_V3COMMON_H_

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

class Ast_முனை_DType;

//============================================================================

class V3Common final {
பொது:
    நிலையான வெறுமை commonAll() VL_MT_DISABLED;
    நிலையான string makeToStringCall(Ast_முனை_DType* nodep, மாறிலி std::string& lhs);
};

#endif  // Guard
