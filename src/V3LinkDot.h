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

#جے_بنا_تعریف VERILATOR_V3LINKDOT_H_
#تعریف_کرو VERILATOR_V3LINKDOT_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"

//============================================================================

تعداد VLink_Dot_قدم : uint8_t { LDS_PRIMARY, LDS_PARAMED, LDS_SCOPED };

class V3LinkDot final {
    ساكن فراغ dumpSubstep(ثابت string& name) VL_MT_DISABLED;
    ساكن فراغ linkDotGuts(AstNetlist* rootp, VLink_Dot_قدم قدم) VL_MT_DISABLED;

عام:
    ساكن فراغ linkDotPrimary(AstNetlist* nodep) VL_MT_DISABLED;
    ساكن فراغ linkDotParamed(AstNetlist* nodep) VL_MT_DISABLED;
    ساكن فراغ linkDotScope(AstNetlist* nodep) VL_MT_DISABLED;
};

#جے_انت  // Guard
