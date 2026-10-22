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

#اگر_غیر_تعریف_شدہ VERILATOR_V3EMITV_H_
#تعریف_کرو VERILATOR_V3EMITV_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

class Ast_گرہ;
class Ast_Sen_درخت;

//============================================================================

class V3EmitV final {
عوامی:
    ساکن خالی verilog_For_درخت(ثابت Ast_گرہ* nodep, std::ostream& os = std::cout);
    ساکن خالی debug_Verilog_For_درخت(ثابت Ast_گرہ* nodep, std::ostream& os);
    ساکن std::string debug_Verilog_For_درخت(ثابت Ast_گرہ* nodep);
    ساکن خالی emitvFiles();
    ساکن خالی debugEmitV(ثابت string& filename);
};

#اگر_ختم  // Guard
