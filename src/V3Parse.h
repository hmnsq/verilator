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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3PARSE_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3PARSE_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"

class AstNetlist;
class VInFilter;
class V3ParseImp;

//============================================================================

class V3Parse final {
    V3ParseImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Parse);

ಸಾರ್ವಜನಿಕ:
    // We must allow reading multiple files into one parser
    V3Parse(AstNetlist* rootp, VInFilter* filterp) VL_MT_DISABLED;
    ~V3Parse() VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    ಶೂನ್ಯ parseFile(File_ಸಾಲು* fileline, ಸ್ಥಿರಾಂಕ string& modname, bool ಒಳ_Library, bool ಒಳ_Lib_Map,
                   ಸ್ಥಿರಾಂಕ string& libname, ಸ್ಥಿರಾಂಕ string& errmsg,
                   ಸ್ಥಿರಾಂಕ std::string& not_ಸಿಕ್ಕಿತು_Name = "") VL_MT_DISABLED;

    // Push preprocessed text to the lexer
    ಸ್ಥಿರ ಶೂನ್ಯ pp_ತಳ್ಳು_ಪಠ್ಯ(V3ParseImp* impp, ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) VL_MT_DISABLED;

    // Suggest PLI functions for spell check
    ಸ್ಥಿರ ಶೂನ್ಯ candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
