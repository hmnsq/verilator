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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3FileLine.h"

class V3ParseImp;
class VInFilter;
class VSpellCheck;

//============================================================================

class V3PreShell final {
    // Static class for calling preprocessor
பொது:
    நிலையான வெறுமை boot() VL_MT_DISABLED;
    நிலையான வெறுமை shutdown() VL_MT_DISABLED;
    நிலையான bool preproc(File_வரி* fl, மாறிலி string& modname, VInFilter* filterp,
                        V3ParseImp* parsep, மாறிலி string& errmsg) VL_MT_DISABLED;
    நிலையான வெறுமை preprocInclude(File_வரி* fl, மாறிலி string& modname) VL_MT_DISABLED;
    நிலையான வெறுமை define_Cmd_வரி(மாறிலி string& name, மாறிலி string& மதிப்பு) VL_MT_DISABLED;
    நிலையான வெறுமை undef(மாறிலி string& name) VL_MT_DISABLED;
    நிலையான வெறுமை dumpDefines(std::ostream& os) VL_MT_DISABLED;
    நிலையான வெறுமை candidateDefines(VSpellCheck* spellerp) VL_MT_DISABLED;
    நிலையான வெறுமை selfTest() VL_MT_DISABLED;
};

#endif  // Guard
