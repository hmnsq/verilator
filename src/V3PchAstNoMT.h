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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Broken.h"
#அந்தர்பாவ "V3Container.h"
#அந்தர்பாவ "V3DebugBisect.h"
#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3FileLine.h"
#அந்தர்பாவ "V3FunctionTraits.h"
#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3Mutex.h"
#அந்தர்பாவ "V3Number.h"
#அந்தர்பாவ "V3Options.h"
#அந்தர்பாவ "V3StdFuture.h"
#அந்தர்பாவ "V3String.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <map>
#அந்தர்பாவ <utility>

#endif  // Guard
