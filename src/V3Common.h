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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

class Ast_കണ്ണി_DType;

//============================================================================

class V3Common final {
പൊതു:
    സ്ഥിതം ശൂന്യം commonAll() VL_MT_DISABLED;
    സ്ഥിതം string makeToStringCall(Ast_കണ്ണി_DType* nodep, സ്ഥിരം std::string& lhs);
};

#endif  // Guard
