// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Link modules/signals together
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

#ifndef VERILATOR_V3LINKLEVEL_H_
#define VERILATOR_V3LINKLEVEL_H_

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Error.h"

#അന്തര്ഭാവ <vector>

//============================================================================

class V3LinkLevel final {
    using ModVec = std::vector<Ast_കണ്ണി_Module*>;

    സ്ഥിതം ശൂന്യം timescaling(സ്ഥിരം ModVec& mods) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം wrap_മുകൾ_Cell(AstNetlist* rootp) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം wrap_മുകൾ_Packages(AstNetlist* rootp) VL_MT_DISABLED;

പൊതു:
    സ്ഥിതം ശൂന്യം modSortByLevel() VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം wrap_മുകൾ(AstNetlist* rootp) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം non_Wrap_മുകൾ(AstNetlist* rootp) VL_MT_DISABLED;
};

#endif  // Guard
