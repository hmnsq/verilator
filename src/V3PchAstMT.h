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

#اگر_تعریف_نشده VERILATOR_V3PCHASTMT_H_
#تعریف_کن VERILATOR_V3PCHASTMT_H_

#اگر_تعریف_شده VL_PCH_INCLUDED
#خطا "Including multiple V3Pch*.h flavors"
#پایان_اگر
#تعریف_کن VL_PCH_INCLUDED

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Broken.h"
#ضمن "V3Container.h"
#ضمن "V3Error.h"
#ضمن "V3FileLine.h"
#ضمن "V3FunctionTraits.h"
#ضمن "V3Global.h"
#ضمن "V3Mutex.h"
#ضمن "V3Number.h"
#ضمن "V3Options.h"
#ضمن "V3StdFuture.h"
#ضمن "V3String.h"

#ضمن <algorithm>

#پایان_اگر  // Guard
