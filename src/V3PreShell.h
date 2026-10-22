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

#अगर_अघोषित VERILATOR_V3PRESHELL_H_
#घोषणा VERILATOR_V3PRESHELL_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Error.h"
#समावेश "V3FileLine.h"

class V3ParseImp;
class VInFilter;
class VSpellCheck;

//============================================================================

class V3PreShell final {
    // Static class for calling preprocessor
खुला:
    अटल व्योम boot() VL_MT_DISABLED;
    अटल व्योम shutdown() VL_MT_DISABLED;
    अटल bool preproc(File_पंक्ति* fl, स्थिर string& modname, VInFilter* filterp,
                        V3ParseImp* parsep, स्थिर string& errmsg) VL_MT_DISABLED;
    अटल व्योम preprocInclude(File_पंक्ति* fl, स्थिर string& modname) VL_MT_DISABLED;
    अटल व्योम define_Cmd_पंक्ति(स्थिर string& name, स्थिर string& मूल्य) VL_MT_DISABLED;
    अटल व्योम undef(स्थिर string& name) VL_MT_DISABLED;
    अटल व्योम dumpDefines(std::ostream& os) VL_MT_DISABLED;
    अटल व्योम candidateDefines(VSpellCheck* spellerp) VL_MT_DISABLED;
    अटल व्योम selfTest() VL_MT_DISABLED;
};

#पूर्ण_अगर  // Guard
