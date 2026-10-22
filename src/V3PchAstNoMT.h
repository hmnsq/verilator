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

#यदि_अपरिभाषितम् VERILATOR_V3PCHASTNOMT_H_
#परिभाषय VERILATOR_V3PCHASTNOMT_H_

#यदि_परिभाषितम् VL_PCH_INCLUDED
#दोषः "Including multiple V3Pch*.h flavors"
#यदि_अन्तः
#परिभाषय VL_PCH_INCLUDED

#परिभाषय VL_MT_DISABLED_संहिता_UNIT 1

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Broken.h"
#अन्तर्भाव "V3Container.h"
#अन्तर्भाव "V3DebugBisect.h"
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
#अन्तर्भाव <map>
#अन्तर्भाव <utility>

#यदि_अन्तः  // Guard
