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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3FileLine.h"

class V3ParseImp;
class VInFilter;
class VSpellCheck;

//============================================================================

class V3PreShell final {
    // Static class for calling preprocessor
ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ boot() VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ shutdown() VL_MT_DISABLED;
    ସ୍ଥିର bool preproc(File_ଧାଡ଼ି* fl, ଅଚଳ string& modname, VInFilter* filterp,
                        V3ParseImp* parsep, ଅଚଳ string& errmsg) VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ preprocInclude(File_ଧାଡ଼ି* fl, ଅଚଳ string& modname) VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ define_Cmd_ଧାଡ଼ି(ଅଚଳ string& name, ଅଚଳ string& ମୂଲ୍ୟ) VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ undef(ଅଚଳ string& name) VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ dumpDefines(std::ostream& os) VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ candidateDefines(VSpellCheck* spellerp) VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ selfTest() VL_MT_DISABLED;
};

#endif  // Guard
