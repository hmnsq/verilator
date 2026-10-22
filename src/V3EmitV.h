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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

class AstNode;
class AstSenTree;

//============================================================================

class V3EmitV final {
عام:
    ساكن فراغ verilogForTree(ثابت AstNode* nodep, std::ostream& os = std::cout);
    ساكن فراغ debugVerilogForTree(ثابت AstNode* nodep, std::ostream& os);
    ساكن std::string debugVerilogForTree(ثابت AstNode* nodep);
    ساكن فراغ emitvFiles();
    ساكن فراغ debugEmitV(ثابت string& filename);
};

#endif  // Guard
