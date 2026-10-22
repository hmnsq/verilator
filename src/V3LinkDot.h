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

#अगर_अघोषित VERILATOR_V3LINKDOT_H_
#घोषणा VERILATOR_V3LINKDOT_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3Error.h"

//============================================================================

क्रमागत VLinkDotStep : uint8_t { LDS_PRIMARY, LDS_PARAMED, LDS_SCOPED };

class V3LinkDot final {
    अटल व्योम dumpSubstep(स्थिर string& name) VL_MT_DISABLED;
    अटल व्योम linkDotGuts(AstNetlist* rootp, VLinkDotStep step) VL_MT_DISABLED;

खुला:
    अटल व्योम linkDotPrimary(AstNetlist* nodep) VL_MT_DISABLED;
    अटल व्योम linkDotParamed(AstNetlist* nodep) VL_MT_DISABLED;
    अटल व्योम linkDotScope(AstNetlist* nodep) VL_MT_DISABLED;
};

#पूर्ण_अगर  // Guard
