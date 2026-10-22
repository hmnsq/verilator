// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Prepare AST for features features
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

#अगर_अघोषित _V3TIMING_H_
#घोषणा _V3TIMING_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"

//============================================================================

class V3Timing final {
खुला:
    अटल व्योम timingAll(AstNetlist* nodep) VL_MT_DISABLED;
    // Scale a delay in the given time unit to time precision ticks, rounding real delays; #1step
    // is one tick
    अटल Ast_गाँठ_व्यंजक* delayToTicks(Ast_गाँठ_व्यंजक* valuep, VTimescale timeunit) VL_MT_DISABLED;
};

#पूर्ण_अगर  // Guard
