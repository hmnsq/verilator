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

#اگر_غیر_تعریف_شدہ VERILATOR_V3PCHASTMT_H_
#تعریف_کرو VERILATOR_V3PCHASTMT_H_

#اگر_تعریف_شدہ VL_PCH_INCLUDED
#غلطی "Including multiple V3Pch*.h flavors"
#اگر_ختم
#تعریف_کرو VL_PCH_INCLUDED

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Broken.h"
#شامل "V3Container.h"
#شامل "V3Error.h"
#شامل "V3FileLine.h"
#شامل "V3FunctionTraits.h"
#شامل "V3Global.h"
#شامل "V3Mutex.h"
#شامل "V3Number.h"
#شامل "V3Options.h"
#شامل "V3StdFuture.h"
#شامل "V3String.h"

#شامل <algorithm>

#اگر_ختم  // Guard
