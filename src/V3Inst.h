// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Break always into sensitivity inst domains
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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3INST_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3INST_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

class Ast_ਸੌਂਪੋ_W;
class AstCell;
class AstNetlist;
class AstPin;

//============================================================================

class V3Inst final {
ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ instAll(AstNetlist* nodep) VL_MT_DISABLED;
    ਸਥਿਰ Ast_ਸੌਂਪੋ_W* pinReconnectSimple(AstPin* pinp, AstCell* cellp, bool forTristate,
                                          bool alwaysCvt = false) VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ checkOutputShort(ਅਚਲ AstPin* nodep) VL_MT_DISABLED;
};

#ਜੇ_ਅੰਤ  // Guard
