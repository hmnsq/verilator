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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Error.h"

//============================================================================

गणना VLinkDotStep : uint8_t { LDS_PRIMARY, LDS_PARAMED, LDS_SCOPED };

class V3LinkDot final {
    स्थिर शून्य dumpSubstep(स्थिरांक string& name) VL_MT_DISABLED;
    स्थिर शून्य linkDotGuts(AstNetlist* rootp, VLinkDotStep step) VL_MT_DISABLED;

सार्वजनिक:
    स्थिर शून्य linkDotPrimary(AstNetlist* nodep) VL_MT_DISABLED;
    स्थिर शून्य linkDotParamed(AstNetlist* nodep) VL_MT_DISABLED;
    स्थिर शून्य linkDotScope(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
