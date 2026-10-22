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

#अगर_अघोषित VERILATOR_V3EMITV_H_
#घोषणा VERILATOR_V3EMITV_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

class Ast_गाँठ;
class Ast_Sen_वृक्ष;

//============================================================================

class V3EmitV final {
खुला:
    अटल व्योम verilog_For_वृक्ष(स्थिर Ast_गाँठ* nodep, std::ostream& os = std::cout);
    अटल व्योम debug_Verilog_For_वृक्ष(स्थिर Ast_गाँठ* nodep, std::ostream& os);
    अटल std::string debug_Verilog_For_वृक्ष(स्थिर Ast_गाँठ* nodep);
    अटल व्योम emitvFiles();
    अटल व्योम debugEmitV(स्थिर string& filename);
};

#पूर्ण_अगर  // Guard
