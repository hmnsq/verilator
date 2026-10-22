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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

class Ast_গ্রন্থি;
class Ast_Sen_বৃক্ষ;

//============================================================================

class V3EmitV final {
সার্বজনিক:
    স্থির শূন্য verilog_For_বৃক্ষ(ধ্রুব Ast_গ্রন্থি* nodep, std::ostream& os = std::cout);
    স্থির শূন্য debug_Verilog_For_বৃক্ষ(ধ্রুব Ast_গ্রন্থি* nodep, std::ostream& os);
    স্থির std::string debug_Verilog_For_বৃক্ষ(ধ্রুব Ast_গ্রন্থি* nodep);
    স্থির শূন্য emitvFiles();
    স্থির শূন্য debugEmitV(ধ্রুব string& filename);
};

#endif  // Guard
