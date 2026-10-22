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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

class Ast_गाठ;
class Ast_Sen_वृक्ष;

//============================================================================

class V3EmitV final {
सार्वजनिक:
    स्थिर शून्य verilog_For_वृक्ष(ध्रुव Ast_गाठ* nodep, std::ostream& os = std::cout);
    स्थिर शून्य debug_Verilog_For_वृक्ष(ध्रुव Ast_गाठ* nodep, std::ostream& os);
    स्थिर std::string debug_Verilog_For_वृक्ष(ध्रुव Ast_गाठ* nodep);
    स्थिर शून्य emitvFiles();
    स्थिर शून्य debugEmitV(ध्रुव string& filename);
};

#endif  // Guard
