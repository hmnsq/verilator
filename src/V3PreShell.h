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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3FileLine.h"

class V3ParseImp;
class VInFilter;
class VSpellCheck;

//============================================================================

class V3PreShell final {
    // Static class for calling preprocessor
সার্ৱজনিক:
    স্থির শূন্য boot() VL_MT_DISABLED;
    স্থির শূন্য shutdown() VL_MT_DISABLED;
    স্থির bool preproc(File_সাৰি* fl, ধ্রুৱ string& modname, VInFilter* filterp,
                        V3ParseImp* parsep, ধ্রুৱ string& errmsg) VL_MT_DISABLED;
    স্থির শূন্য preprocInclude(File_সাৰি* fl, ধ্রুৱ string& modname) VL_MT_DISABLED;
    স্থির শূন্য define_Cmd_সাৰি(ধ্রুৱ string& name, ধ্রুৱ string& মান) VL_MT_DISABLED;
    স্থির শূন্য undef(ধ্রুৱ string& name) VL_MT_DISABLED;
    স্থির শূন্য dumpDefines(std::ostream& os) VL_MT_DISABLED;
    স্থির শূন্য candidateDefines(VSpellCheck* spellerp) VL_MT_DISABLED;
    স্থির শূন্য selfTest() VL_MT_DISABLED;
};

#endif  // Guard
