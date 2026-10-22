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

#که_نه_تعریف_شوی VERILATOR_V3PRESHELL_H_
#تعریف_کړه VERILATOR_V3PRESHELL_H_

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
    ساكن bool preproc(File_کرښه* fl, ثابت string& modname, VInFilter* filterp,
                        V3ParseImp* parsep, ثابت string& errmsg) VL_MT_DISABLED;
    ساكن فراغ preprocInclude(File_کرښه* fl, ثابت string& modname) VL_MT_DISABLED;
    ساكن فراغ define_Cmd_کرښه(ثابت string& name, ثابت string& ارزښت) VL_MT_DISABLED;
    ساكن فراغ undef(ثابت string& name) VL_MT_DISABLED;
    ساكن فراغ dump_تعریفوي(std::ostream& os) VL_MT_DISABLED;
    ساكن فراغ candidate_تعریفوي(VSpellCheck* spellerp) VL_MT_DISABLED;
    ساكن فراغ selfTest() VL_MT_DISABLED;
};

#د_که_پای  // Guard
