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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Broken.h"
#ಅನ್ತರ್ಭಾವ "V3Container.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3FileLine.h"
#ಅನ್ತರ್ಭಾವ "V3FunctionTraits.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Mutex.h"
#ಅನ್ತರ್ಭಾವ "V3Number.h"
#ಅನ್ತರ್ಭಾವ "V3Options.h"
#ಅನ್ತರ್ಭಾವ "V3StdFuture.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"

#ಅನ್ತರ್ಭಾವ <algorithm>

#endif  // Guard
