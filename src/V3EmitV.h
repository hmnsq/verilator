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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

class Ast_ગાંઠ;
class Ast_Sen_વૃક્ષ;

//============================================================================

class V3EmitV final {
જાહેર:
    સ્થિર શૂન્ય verilog_For_વૃક્ષ(અચલ Ast_ગાંઠ* nodep, std::ostream& os = std::cout);
    સ્થિર શૂન્ય debug_Verilog_For_વૃક્ષ(અચલ Ast_ગાંઠ* nodep, std::ostream& os);
    સ્થિર std::string debug_Verilog_For_વૃક્ષ(અચલ Ast_ગાંઠ* nodep);
    સ્થિર શૂન્ય emitvFiles();
    સ્થિર શૂન્ય debugEmitV(અચલ string& filename);
};

#endif  // Guard
