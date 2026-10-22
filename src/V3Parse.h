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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3PARSE_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3PARSE_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"

class AstNetlist;
class VInFilter;
class V3ParseImp;

//============================================================================

class V3Parse final {
    V3ParseImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Parse);

ਜਨਤਕ:
    // We must allow reading multiple files into one parser
    V3Parse(AstNetlist* rootp, VInFilter* filterp) VL_MT_DISABLED;
    ~V3Parse() VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    ਖਾਲੀ parseFile(File_ਸਤਰ* fileline, ਅਚਲ string& modname, bool ਵਿੱਚ_Library, bool ਵਿੱਚ_Lib_Map,
                   ਅਚਲ string& libname, ਅਚਲ string& errmsg,
                   ਅਚਲ std::string& not_ਮਿਲਿਆ_Name = "") VL_MT_DISABLED;

    // Push preprocessed text to the lexer
    ਸਥਿਰ ਖਾਲੀ pp_ਧੱਕੋ_ਪਾਠ(V3ParseImp* impp, ਅਚਲ string& ਪਾਠ) VL_MT_DISABLED;

    // Suggest PLI functions for spell check
    ਸਥਿਰ ਖਾਲੀ candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
};

#ਜੇ_ਅੰਤ  // Guard
