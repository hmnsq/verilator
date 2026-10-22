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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3Global.h"

class AstNetlist;
class VInFilter;
class V3ParseImp;

//============================================================================

class V3Parse final {
    V3ParseImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Parse);

সার্বজনিক:
    // We must allow reading multiple files into one parser
    V3Parse(AstNetlist* rootp, VInFilter* filterp) VL_MT_DISABLED;
    ~V3Parse() VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    শূন্য parseFile(File_সারি* fileline, ধ্রুব string& modname, bool inLibrary, bool inLibMap,
                   ধ্রুব string& libname, ধ্রুব string& errmsg,
                   ধ্রুব std::string& notFoundName = "") VL_MT_DISABLED;

    // Push preprocessed text to the lexer
    স্থির শূন্য pp_ঠেলো_Text(V3ParseImp* impp, ধ্রুব string& text) VL_MT_DISABLED;

    // Suggest PLI functions for spell check
    স্থির শূন্য candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
};

#endif  // Guard
