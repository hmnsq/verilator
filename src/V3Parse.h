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

#جے_بنا_تعریف VERILATOR_V3PARSE_H_
#تعریف_کرو VERILATOR_V3PARSE_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Error.h"
#ضمن "V3Global.h"

class AstNetlist;
class VInFilter;
class V3ParseImp;

//============================================================================

class V3Parse final {
    V3ParseImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Parse);

عام:
    // We must allow reading multiple files into one parser
    V3Parse(AstNetlist* rootp, VInFilter* filterp) VL_MT_DISABLED;
    ~V3Parse() VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    فراغ parseFile(File_سطر* fileline, ثابت string& modname, bool وچ_Library, bool وچ_Lib_Map,
                   ثابت string& libname, ثابت string& errmsg,
                   ثابت std::string& not_لبھیا_Name = "") VL_MT_DISABLED;

    // Push preprocessed text to the lexer
    ساكن فراغ pp_ڈالو_متن(V3ParseImp* impp, ثابت string& متن) VL_MT_DISABLED;

    // Suggest PLI functions for spell check
    ساكن فراغ candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
};

#جے_انت  // Guard
