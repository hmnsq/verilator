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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Error.h"

//============================================================================

எண்ணிக்கை VLinkDotStep : uint8_t { LDS_PRIMARY, LDS_PARAMED, LDS_SCOPED };

class V3LinkDot final {
    நிலையான வெறுமை dumpSubstep(மாறிலி string& name) VL_MT_DISABLED;
    நிலையான வெறுமை linkDotGuts(AstNetlist* rootp, VLinkDotStep step) VL_MT_DISABLED;

பொது:
    நிலையான வெறுமை linkDotPrimary(AstNetlist* nodep) VL_MT_DISABLED;
    நிலையான வெறுமை linkDotParamed(AstNetlist* nodep) VL_MT_DISABLED;
    நிலையான வெறுமை linkDotScope(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
