// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Dump AstNodeExpr patterns for analysis
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

#ifndef VERILATOR_V3ASTPATTERNS_H_
#define VERILATOR_V3ASTPATTERNS_H_

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

class AstNetlist;

//============================================================================

class V3AstPatterns final {
பொது:
    // Accumulate the patterns in the netlist, and dump the collected
    // statistics to '<prefix>__ast_patterns_<suffix>.txt'.
    நிலையான வெறுமை dumpAll(மாறிலி AstNetlist* nodep, மாறிலி std::string& suffix) VL_MT_DISABLED;
};

#endif  // Guard
