// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit Verilog code for module tree
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

#ifndef VERILATOR_V3EMITV_H_
#define VERILATOR_V3EMITV_H_

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

class Ast_കണ്ണി;
class Ast_Sen_വൃക്ഷം;

//============================================================================

class V3EmitV final {
പൊതു:
    സ്ഥിതം ശൂന്യം verilog_For_വൃക്ഷം(സ്ഥിരം Ast_കണ്ണി* nodep, std::ostream& os = std::cout);
    സ്ഥിതം ശൂന്യം debug_Verilog_For_വൃക്ഷം(സ്ഥിരം Ast_കണ്ണി* nodep, std::ostream& os);
    സ്ഥിതം std::string debug_Verilog_For_വൃക്ഷം(സ്ഥിരം Ast_കണ്ണി* nodep);
    സ്ഥിതം ശൂന്യം emitvFiles();
    സ്ഥിതം ശൂന്യം debugEmitV(സ്ഥിരം string& filename);
};

#endif  // Guard
