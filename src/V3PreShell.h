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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3FileLine.h"

class V3ParseImp;
class VInFilter;
class VSpellCheck;

//============================================================================

class V3PreShell final {
    // Static class for calling preprocessor
જાહેર:
    સ્થિર શૂન્ય boot() VL_MT_DISABLED;
    સ્થિર શૂન્ય shutdown() VL_MT_DISABLED;
    સ્થિર bool preproc(File_લીટી* fl, અચલ string& modname, VInFilter* filterp,
                        V3ParseImp* parsep, અચલ string& errmsg) VL_MT_DISABLED;
    સ્થિર શૂન્ય preprocInclude(File_લીટી* fl, અચલ string& modname) VL_MT_DISABLED;
    સ્થિર શૂન્ય define_Cmd_લીટી(અચલ string& name, અચલ string& કિંમત) VL_MT_DISABLED;
    સ્થિર શૂન્ય undef(અચલ string& name) VL_MT_DISABLED;
    સ્થિર શૂન્ય dumpDefines(std::ostream& os) VL_MT_DISABLED;
    સ્થિર શૂન્ય candidateDefines(VSpellCheck* spellerp) VL_MT_DISABLED;
    સ્થિર શૂન્ય selfTest() VL_MT_DISABLED;
};

#endif  // Guard
