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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3Global.h"

class AstNetlist;
class VInFilter;
class V3ParseImp;

//============================================================================

class V3Parse final {
    V3ParseImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Parse);

सार्वजनिक:
    // We must allow reading multiple files into one parser
    V3Parse(AstNetlist* rootp, VInFilter* filterp) VL_MT_DISABLED;
    ~V3Parse() VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    शून्य parseFile(File_ओळ* fileline, ध्रुव string& modname, bool inLibrary, bool inLibMap,
                   ध्रुव string& libname, ध्रुव string& errmsg,
                   ध्रुव std::string& notFoundName = "") VL_MT_DISABLED;

    // Push preprocessed text to the lexer
    स्थिर शून्य pp_ढकला_Text(V3ParseImp* impp, ध्रुव string& text) VL_MT_DISABLED;

    // Suggest PLI functions for spell check
    स्थिर शून्य candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
};

#endif  // Guard
