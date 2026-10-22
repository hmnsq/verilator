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

#ifndef VERILATOR_V3LINKDOT_H_
#define VERILATOR_V3LINKDOT_H_

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Error.h"

//============================================================================

എണ്ണൽ VLinkDotStep : uint8_t { LDS_PRIMARY, LDS_PARAMED, LDS_SCOPED };

class V3LinkDot final {
    സ്ഥിതം ശൂന്യം dumpSubstep(സ്ഥിരം string& name) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം linkDotGuts(AstNetlist* rootp, VLinkDotStep step) VL_MT_DISABLED;

പൊതു:
    സ്ഥിതം ശൂന്യം linkDotPrimary(AstNetlist* nodep) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം linkDotParamed(AstNetlist* nodep) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം linkDotScope(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
