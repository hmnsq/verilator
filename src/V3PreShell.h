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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3FileLine.h"

class V3ParseImp;
class VInFilter;
class VSpellCheck;

//============================================================================

class V3PreShell final {
    // Static class for calling preprocessor
सार्वजनिक:
    स्थिर शून्य boot() VL_MT_DISABLED;
    स्थिर शून्य shutdown() VL_MT_DISABLED;
    स्थिर bool preproc(File_पंक्ति* fl, ध्रुव string& modname, VInFilter* filterp,
                        V3ParseImp* parsep, ध्रुव string& errmsg) VL_MT_DISABLED;
    स्थिर शून्य preprocInclude(File_पंक्ति* fl, ध्रुव string& modname) VL_MT_DISABLED;
    स्थिर शून्य define_Cmd_पंक्ति(ध्रुव string& name, ध्रुव string& मूल्य) VL_MT_DISABLED;
    स्थिर शून्य undef(ध्रुव string& name) VL_MT_DISABLED;
    स्थिर शून्य dumpDefines(std::ostream& os) VL_MT_DISABLED;
    स्थिर शून्य candidateDefines(VSpellCheck* spellerp) VL_MT_DISABLED;
    स्थिर शून्य selfTest() VL_MT_DISABLED;
};

#endif  // Guard
