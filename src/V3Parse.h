// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Reading of Verilog files
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ifndef VERILATOR_V3PARSE_H_
#define VERILATOR_V3PARSE_H_

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3Global.h"

class AstNetlist;
class VInFilter;
class V3ParseImp;

//============================================================================

class V3Parse final {
    V3ParseImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Parse);

പൊതു:
    // We must allow reading multiple files into one parser
    V3Parse(AstNetlist* rootp, VInFilter* filterp) VL_MT_DISABLED;
    ~V3Parse() VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    ശൂന്യം parseFile(File_വരി* fileline, സ്ഥിരം string& modname, bool inLibrary, bool inLibMap,
                   സ്ഥിരം string& libname, സ്ഥിരം string& errmsg,
                   സ്ഥിരം std::string& notFoundName = "") VL_MT_DISABLED;

    // Push preprocessed text to the lexer
    സ്ഥിതം ശൂന്യം pp_തള്ളുക_Text(V3ParseImp* impp, സ്ഥിരം string& text) VL_MT_DISABLED;

    // Suggest PLI functions for spell check
    സ്ഥിതം ശൂന്യം candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
};

#endif  // Guard
