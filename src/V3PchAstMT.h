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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Broken.h"
#अन्तर्भाव "V3Container.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3FileLine.h"
#अन्तर्भाव "V3FunctionTraits.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Mutex.h"
#अन्तर्भाव "V3Number.h"
#अन्तर्भाव "V3Options.h"
#अन्तर्भाव "V3StdFuture.h"
#अन्तर्भाव "V3String.h"

#अन्तर्भाव <algorithm>

#endif  // Guard
