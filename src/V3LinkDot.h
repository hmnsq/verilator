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

#اگر_غیر_تعریف_شدہ VERILATOR_V3LINKDOT_H_
#تعریف_کرو VERILATOR_V3LINKDOT_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Error.h"

//============================================================================

شمار VLink_Dot_قدم : uint8_t { LDS_PRIMARY, LDS_PARAMED, LDS_SCOPED };

class V3LinkDot final {
    ساکن خالی dumpSubstep(ثابت string& name) VL_MT_DISABLED;
    ساکن خالی linkDotGuts(AstNetlist* rootp, VLink_Dot_قدم قدم) VL_MT_DISABLED;

عوامی:
    ساکن خالی linkDotPrimary(AstNetlist* nodep) VL_MT_DISABLED;
    ساکن خالی linkDotParamed(AstNetlist* nodep) VL_MT_DISABLED;
    ساکن خالی linkDotScope(AstNetlist* nodep) VL_MT_DISABLED;
};

#اگر_ختم  // Guard
