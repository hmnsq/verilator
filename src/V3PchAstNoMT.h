// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// Code available from: https://verilator.org
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// DESCRIPTION: Verilator: Precompilable header of V3Ast, non-multithreaded
//
//*************************************************************************

#ifndef VERILATOR_V3PCHASTNOMT_H_
#define VERILATOR_V3PCHASTNOMT_H_

#ifdef VL_PCH_INCLUDED
#error "Including multiple V3Pch*.h flavors"
#endif
#define VL_PCH_INCLUDED

#define VL_MT_DISABLED_CODE_UNIT 1

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Broken.h"
#અન્તર્ભાવ "V3Container.h"
#અન્તર્ભાવ "V3DebugBisect.h"
#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3FileLine.h"
#અન્તર્ભાવ "V3FunctionTraits.h"
#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3Mutex.h"
#અન્તર્ભાવ "V3Number.h"
#અન્તર્ભાવ "V3Options.h"
#અન્તર્ભાવ "V3StdFuture.h"
#અન્તર્ભાવ "V3String.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <map>
#અન્તર્ભાવ <utility>

#endif  // Guard
