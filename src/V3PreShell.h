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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3PRESHELL_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3PRESHELL_H_

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
    ಸ್ಥಿರ bool preproc(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& modname, VInFilter* filterp,
                        V3ParseImp* parsep, ಸ್ಥಿರಾಂಕ string& errmsg) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ preprocInclude(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& modname) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ define_Cmd_ಸಾಲು(ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& ಮೌಲ್ಯ) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ undef(ಸ್ಥಿರಾಂಕ string& name) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ dump_ನಿರೂಪಿಸುತ್ತದೆ(std::ostream& os) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ candidate_ನಿರೂಪಿಸುತ್ತದೆ(VSpellCheck* spellerp) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ selfTest() VL_MT_DISABLED;
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
