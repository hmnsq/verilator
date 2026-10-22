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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Error.h"

//============================================================================

ગણના VLinkDotStep : uint8_t { LDS_PRIMARY, LDS_PARAMED, LDS_SCOPED };

class V3LinkDot final {
    સ્થિર શૂન્ય dumpSubstep(અચલ string& name) VL_MT_DISABLED;
    સ્થિર શૂન્ય linkDotGuts(AstNetlist* rootp, VLinkDotStep step) VL_MT_DISABLED;

જાહેર:
    સ્થિર શૂન્ય linkDotPrimary(AstNetlist* nodep) VL_MT_DISABLED;
    સ્થિર શૂન્ય linkDotParamed(AstNetlist* nodep) VL_MT_DISABLED;
    સ્થિર શૂન્ય linkDotScope(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
