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

#अगर_अघोषित VERILATOR_V3PARSE_H_
#घोषणा VERILATOR_V3PARSE_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Error.h"
#समावेश "V3Global.h"

class AstNetlist;
class VInFilter;
class V3ParseImp;

//============================================================================

class V3Parse final {
    V3ParseImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Parse);

खुला:
    // We must allow reading multiple files into one parser
    V3Parse(AstNetlist* rootp, VInFilter* filterp) VL_MT_DISABLED;
    ~V3Parse() VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    व्योम parseFile(File_पंक्ति* fileline, स्थिर string& modname, bool inLibrary, bool inLibMap,
                   स्थिर string& libname, स्थिर string& errmsg,
                   स्थिर std::string& notFoundName = "") VL_MT_DISABLED;

    // Push preprocessed text to the lexer
    अटल व्योम pp_धकेलो_Text(V3ParseImp* impp, स्थिर string& text) VL_MT_DISABLED;

    // Suggest PLI functions for spell check
    अटल व्योम candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
};

#पूर्ण_अगर  // Guard
