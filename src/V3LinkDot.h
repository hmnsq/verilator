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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"

//============================================================================

ಗಣನಾ VLinkDotStep : uint8_t { LDS_PRIMARY, LDS_PARAMED, LDS_SCOPED };

class V3LinkDot final {
    ಸ್ಥಿರ ಶೂನ್ಯ dumpSubstep(ಧ್ರುವ string& name) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ linkDotGuts(AstNetlist* rootp, VLinkDotStep step) VL_MT_DISABLED;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ linkDotPrimary(AstNetlist* nodep) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ linkDotParamed(AstNetlist* nodep) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ linkDotScope(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
