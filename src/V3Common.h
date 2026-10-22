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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3COMMON_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3COMMON_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

class Ast_ਗੰਢ_DType;

//============================================================================

class V3Common final {
ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ commonAll() VL_MT_DISABLED;
    ਸਥਿਰ string make_ਮੰਜ਼ਿਲ_String_Call(Ast_ਗੰਢ_DType* nodep, ਅਚਲ std::string& lhs);
};

#ਜੇ_ਅੰਤ  // Guard
