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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3FileLine.h"

class V3ParseImp;
class VInFilter;
class VSpellCheck;

//============================================================================

class V3PreShell final {
    // Static class for calling preprocessor
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ boot() VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ shutdown() VL_MT_DISABLED;
    ಸ್ಥಿರ bool preproc(File_ಸಾಲು* fl, ಧ್ರುವ string& modname, VInFilter* filterp,
                        V3ParseImp* parsep, ಧ್ರುವ string& errmsg) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ preprocInclude(File_ಸಾಲು* fl, ಧ್ರುವ string& modname) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ define_Cmd_ಸಾಲು(ಧ್ರುವ string& name, ಧ್ರುವ string& ಮೌಲ್ಯ) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ undef(ಧ್ರುವ string& name) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ dumpDefines(std::ostream& os) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ candidateDefines(VSpellCheck* spellerp) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ selfTest() VL_MT_DISABLED;
};

#endif  // Guard
