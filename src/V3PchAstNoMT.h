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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Broken.h"
#അന്തര്ഭാവ "V3Container.h"
#അന്തര്ഭാവ "V3DebugBisect.h"
#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3FileLine.h"
#അന്തര്ഭാവ "V3FunctionTraits.h"
#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3Mutex.h"
#അന്തര്ഭാവ "V3Number.h"
#അന്തര്ഭാവ "V3Options.h"
#അന്തര്ഭാവ "V3StdFuture.h"
#അന്തര്ഭാവ "V3String.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <utility>

#endif  // Guard
