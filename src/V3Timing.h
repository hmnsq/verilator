// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Prepare AST for features features
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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ _V3TIMING_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ _V3TIMING_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"

//============================================================================

class V3Timing final {
ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ timingAll(AstNetlist* nodep) VL_MT_DISABLED;
    // Scale a delay in the given time unit to time precision ticks, rounding real delays; #1step
    // is one tick
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* delay_ਮੰਜ਼ਿਲ_Ticks(Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep, VTimescale timeunit) VL_MT_DISABLED;
};

#ਜੇ_ਅੰਤ  // Guard
