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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3Error.h"

//============================================================================

গণনা VLinkDotStep : uint8_t { LDS_PRIMARY, LDS_PARAMED, LDS_SCOPED };

class V3LinkDot final {
    স্থির শূন্য dumpSubstep(ধ্রুৱ string& name) VL_MT_DISABLED;
    স্থির শূন্য linkDotGuts(AstNetlist* rootp, VLinkDotStep step) VL_MT_DISABLED;

সার্ৱজনিক:
    স্থির শূন্য linkDotPrimary(AstNetlist* nodep) VL_MT_DISABLED;
    স্থির শূন্য linkDotParamed(AstNetlist* nodep) VL_MT_DISABLED;
    স্থির শূন্য linkDotScope(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
