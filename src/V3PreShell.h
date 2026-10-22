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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3FileLine.h"

class V3ParseImp;
class VInFilter;
class VSpellCheck;

//============================================================================

class V3PreShell final {
    // Static class for calling preprocessor
പൊതു:
    സ്ഥിതം ശൂന്യം boot() VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം shutdown() VL_MT_DISABLED;
    സ്ഥിതം bool preproc(File_വരി* fl, സ്ഥിരം string& modname, VInFilter* filterp,
                        V3ParseImp* parsep, സ്ഥിരം string& errmsg) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം preprocInclude(File_വരി* fl, സ്ഥിരം string& modname) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം define_Cmd_വരി(സ്ഥിരം string& name, സ്ഥിരം string& മൂല്യം) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം undef(സ്ഥിരം string& name) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം dumpDefines(std::ostream& os) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം candidateDefines(VSpellCheck* spellerp) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം selfTest() VL_MT_DISABLED;
};

#endif  // Guard
