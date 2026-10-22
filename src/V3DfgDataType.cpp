// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Type system used by DFG
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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Dfg.h"
#അന്തര്ഭാവ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgDataType

സ്ഥിരം DfgDataType* DfgDataType::s_nullTypep{nullptr};
std::unordered_map<uint32_t, സ്ഥിരം DfgDataType*> DfgDataType::s_packedTypes{};

//------------------------------------------------------------------------------
// Type checker - for internal validation only

ശൂന്യം V3DfgPasses::typeCheck(സ്ഥിരം DfgGraph& dfg) {
    dfg.forEachVertex([&](സ്ഥിരം DfgVertex& vtx) { vtx.typeCheck(dfg); });
}
