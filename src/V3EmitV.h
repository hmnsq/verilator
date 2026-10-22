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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

class Ast_முனை;
class Ast_Sen_மரம்;

//============================================================================

class V3EmitV final {
பொது:
    நிலையான வெறுமை verilog_For_மரம்(மாறிலி Ast_முனை* nodep, std::ostream& os = std::cout);
    நிலையான வெறுமை debug_Verilog_For_மரம்(மாறிலி Ast_முனை* nodep, std::ostream& os);
    நிலையான std::string debug_Verilog_For_மரம்(மாறிலி Ast_முனை* nodep);
    நிலையான வெறுமை emitvFiles();
    நிலையான வெறுமை debugEmitV(மாறிலி string& filename);
};

#endif  // Guard
