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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3PRESHELL_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3PRESHELL_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3FileLine.h"

class V3ParseImp;
class VInFilter;
class VSpellCheck;

//============================================================================

class V3PreShell final {
    // Static class for calling preprocessor
ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ boot() VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ shutdown() VL_MT_DISABLED;
    ਸਥਿਰ bool preproc(File_ਸਤਰ* fl, ਅਚਲ string& modname, VInFilter* filterp,
                        V3ParseImp* parsep, ਅਚਲ string& errmsg) VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ preprocInclude(File_ਸਤਰ* fl, ਅਚਲ string& modname) VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ define_Cmd_ਸਤਰ(ਅਚਲ string& name, ਅਚਲ string& ਮੁੱਲ) VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ undef(ਅਚਲ string& name) VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ dump_ਨਿਰਧਾਰਤ_ਕਰਦਾ(std::ostream& os) VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ candidate_ਨਿਰਧਾਰਤ_ਕਰਦਾ(VSpellCheck* spellerp) VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ selfTest() VL_MT_DISABLED;
};

#ਜੇ_ਅੰਤ  // Guard
