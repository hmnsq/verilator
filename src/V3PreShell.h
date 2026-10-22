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

#اگر_غیر_تعریف_شدہ VERILATOR_V3PRESHELL_H_
#تعریف_کرو VERILATOR_V3PRESHELL_H_

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
عوامی:
    ساکن خالی boot() VL_MT_DISABLED;
    ساکن خالی shutdown() VL_MT_DISABLED;
    ساکن bool preproc(File_سطر* fl, ثابت string& modname, VInFilter* filterp,
                        V3ParseImp* parsep, ثابت string& errmsg) VL_MT_DISABLED;
    ساکن خالی preprocInclude(File_سطر* fl, ثابت string& modname) VL_MT_DISABLED;
    ساکن خالی define_Cmd_سطر(ثابت string& name, ثابت string& قدر) VL_MT_DISABLED;
    ساکن خالی undef(ثابت string& name) VL_MT_DISABLED;
    ساکن خالی dump_متعین_کرتا(std::ostream& os) VL_MT_DISABLED;
    ساکن خالی candidate_متعین_کرتا(VSpellCheck* spellerp) VL_MT_DISABLED;
    ساکن خالی selfTest() VL_MT_DISABLED;
};

#اگر_ختم  // Guard
