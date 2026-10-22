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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

class Ast_عقدة_DType;

//============================================================================

class V3Common final {
عام:
    ساكن فراغ commonAll() VL_MT_DISABLED;
    ساكن string makeToStringCall(Ast_عقدة_DType* nodep, ثابت std::string& lhs);
};

#endif  // Guard
