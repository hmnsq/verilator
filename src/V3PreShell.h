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

#اگر_تعریف_نشده VERILATOR_V3PRESHELL_H_
#تعریف_کن VERILATOR_V3PRESHELL_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Error.h"
#ضمن "V3FileLine.h"

class V3ParseImp;
class VInFilter;
class VSpellCheck;

//============================================================================

class V3PreShell final {
    // Static class for calling preprocessor
عام:
    ساكن فراغ boot() VL_MT_DISABLED;
    ساكن فراغ shutdown() VL_MT_DISABLED;
    ساكن bool preproc(File_سطر* fl, ثابت string& modname, VInFilter* filterp,
                        V3ParseImp* parsep, ثابت string& errmsg) VL_MT_DISABLED;
    ساكن فراغ preprocInclude(File_سطر* fl, ثابت string& modname) VL_MT_DISABLED;
    ساكن فراغ define_Cmd_سطر(ثابت string& name, ثابت string& مقدار) VL_MT_DISABLED;
    ساكن فراغ undef(ثابت string& name) VL_MT_DISABLED;
    ساكن فراغ dump_تعریف_میکند(std::ostream& os) VL_MT_DISABLED;
    ساكن فراغ candidate_تعریف_میکند(VSpellCheck* spellerp) VL_MT_DISABLED;
    ساكن فراغ selfTest() VL_MT_DISABLED;
};

#پایان_اگر  // Guard
