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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

class Ast_গ্ৰন্থি_DType;

//============================================================================

class V3Common final {
সার্ৱজনিক:
    স্থির শূন্য commonAll() VL_MT_DISABLED;
    স্থির string makeToStringCall(Ast_গ্ৰন্থি_DType* nodep, ধ্রুৱ std::string& lhs);
};

#endif  // Guard
