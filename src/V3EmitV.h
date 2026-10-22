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

#నిర్వచితం_కాకపోతే VERILATOR_V3EMITV_H_
#నిర్వచించు VERILATOR_V3EMITV_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

class Ast_కణుపు;
class Ast_Sen_వృక్షం;

//============================================================================

class V3EmitV final {
బహిరంగ:
    స్థిర శూన్య verilog_For_వృక్షం(స్థిరరాశి Ast_కణుపు* nodep, std::ostream& os = std::cout);
    స్థిర శూన్య debug_Verilog_For_వృక్షం(స్థిరరాశి Ast_కణుపు* nodep, std::ostream& os);
    స్థిర std::string debug_Verilog_For_వృక్షం(స్థిరరాశి Ast_కణుపు* nodep);
    స్థిర శూన్య emitvFiles();
    స్థిర శూన్య debugEmitV(స్థిరరాశి string& filename);
};

#అయితే_ముగింపు  // Guard
