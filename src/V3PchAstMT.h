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
// DESCRIPTION: Verilator: Precompilable header of V3Ast, multithreaded
//
//*************************************************************************

#ifndef VERILATOR_V3PCHASTMT_H_
#define VERILATOR_V3PCHASTMT_H_

#ifdef VL_PCH_INCLUDED
#error "Including multiple V3Pch*.h flavors"
#endif
#define VL_PCH_INCLUDED

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3Broken.h"
#অন্তর্ভাৱ "V3Container.h"
#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3FileLine.h"
#অন্তর্ভাৱ "V3FunctionTraits.h"
#অন্তর্ভাৱ "V3Global.h"
#অন্তর্ভাৱ "V3Mutex.h"
#অন্তর্ভাৱ "V3Number.h"
#অন্তর্ভাৱ "V3Options.h"
#অন্তর্ভাৱ "V3StdFuture.h"
#অন্তর্ভাৱ "V3String.h"

#অন্তর্ভাৱ <algorithm>

#endif  // Guard
