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

#شامل "config_build.h"
#شامل "verilatedos.h"

class Ast_گره_DType;

//============================================================================

class V3Common final {
عمومی:
    ایستا تهی commonAll() VL_MT_DISABLED;
    ایستا string makeToStringCall(Ast_گره_DType* nodep, ثابت std::string& lhs);
};

#endif  // Guard
