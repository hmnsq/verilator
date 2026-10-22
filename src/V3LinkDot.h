// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Link XREF signals/functions together
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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3LINKDOT_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3LINKDOT_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"

//============================================================================

ਗਿਣਤੀ VLink_Dot_ਕਦਮ : uint8_t { LDS_PRIMARY, LDS_PARAMED, LDS_SCOPED };

class V3LinkDot final {
    ਸਥਿਰ ਖਾਲੀ dumpSubstep(ਅਚਲ string& name) VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ linkDotGuts(AstNetlist* rootp, VLink_Dot_ਕਦਮ ਕਦਮ) VL_MT_DISABLED;

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ linkDotPrimary(AstNetlist* nodep) VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ linkDotParamed(AstNetlist* nodep) VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ linkDotScope(AstNetlist* nodep) VL_MT_DISABLED;
};

#ਜੇ_ਅੰਤ  // Guard
