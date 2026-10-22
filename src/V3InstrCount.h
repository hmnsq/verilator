// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Estimate instruction count to run the logic
//                         we would generate for any given AST subtree.
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

#నిర్వచితం_కాకపోతే VERILATOR_V3INSTRCOUNT_H_
#నిర్వచించు VERILATOR_V3INSTRCOUNT_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

class Ast_కణుపు;

class V3_Instr_లెక్క final {
బహిరంగ:
    // Return the estimate count of instructions we'd incur while running
    // code in and under nodep.
    //
    // This is a rough estimate; we don't know what path we'll take through
    // conditionals in nodep, so we assume we take the longest path.
    //
    // If nodep is an AstActive, returns 0.
    // If nodep contains nested AstActives, raises an error.
    //
    // If assertNoDups is true, marks user1 on each AstNode scanned.  Then
    // if we see the same node twice (across more than one call to count,
    // potentially) raises an error.
    // Optional osp is stream to dump critical path to.
    స్థిర uint32_t లెక్క(Ast_కణుపు* nodep, bool assertNoDups,
                          std::ostream* osp = nullptr) VL_MT_DISABLED;
};

#అయితే_ముగింపు  // guard
