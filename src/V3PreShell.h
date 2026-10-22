// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Preprocessing wrapper program
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2004-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ifndef VERILATOR_V3PRESHELL_H_
#define VERILATOR_V3PRESHELL_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Error.h"
#شامل "V3FileLine.h"

class V3ParseImp;
class VInFilter;
class VSpellCheck;

//============================================================================

class V3PreShell final {
    // Static class for calling preprocessor
عمومی:
    ایستا تهی boot() VL_MT_DISABLED;
    ایستا تهی shutdown() VL_MT_DISABLED;
    ایستا bool preproc(File_سطر* fl, ثابت string& modname, VInFilter* filterp,
                        V3ParseImp* parsep, ثابت string& errmsg) VL_MT_DISABLED;
    ایستا تهی preprocInclude(File_سطر* fl, ثابت string& modname) VL_MT_DISABLED;
    ایستا تهی define_Cmd_سطر(ثابت string& name, ثابت string& مقدار) VL_MT_DISABLED;
    ایستا تهی undef(ثابت string& name) VL_MT_DISABLED;
    ایستا تهی dumpDefines(std::ostream& os) VL_MT_DISABLED;
    ایستا تهی candidateDefines(VSpellCheck* spellerp) VL_MT_DISABLED;
    ایستا تهی selfTest() VL_MT_DISABLED;
};

#endif  // Guard
