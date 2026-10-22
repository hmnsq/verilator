// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Estimate stackuction count to run the logic
//                         we would generate for any given AST subtree.
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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3STACKCOUNT_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3STACKCOUNT_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

class Ast_ਗੰਢ;

class V3_ਢੇਰ_ਗਿਣਤੀ final {
ਜਨਤਕ:
    // Return the estimate count of bytes needed in stack we'd incur while
    // running code in and under nodep.
    //
    // This is a rough estimate; we don't know what path we'll take through
    // conditionals in nodep, so we assume we take the longest path.
    ਸਥਿਰ uint64_t ਗਿਣਤੀ_2(Ast_ਗੰਢ* nodep) VL_MT_DISABLED;
};

#ਜੇ_ਅੰਤ  // guard
