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

#నిర్వచితం_కాకపోతే VERILATOR_V3PARSE_H_
#నిర్వచించు VERILATOR_V3PARSE_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3Global.h"

class AstNetlist;
class VInFilter;
class V3ParseImp;

//============================================================================

class V3Parse final {
    V3ParseImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Parse);

బహిరంగ:
    // We must allow reading multiple files into one parser
    V3Parse(AstNetlist* rootp, VInFilter* filterp) VL_MT_DISABLED;
    ~V3Parse() VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    శూన్య parseFile(File_పంక్తి* fileline, స్థిరరాశి string& modname, bool లోపల_Library, bool లోపల_Lib_Map,
                   స్థిరరాశి string& libname, స్థిరరాశి string& errmsg,
                   స్థిరరాశి std::string& not_దొరికింది_Name = "") VL_MT_DISABLED;

    // Push preprocessed text to the lexer
    స్థిర శూన్య pp_నెట్టు_పాఠ్యం(V3ParseImp* impp, స్థిరరాశి string& పాఠ్యం) VL_MT_DISABLED;

    // Suggest PLI functions for spell check
    స్థిర శూన్య candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
};

#అయితే_ముగింపు  // Guard
