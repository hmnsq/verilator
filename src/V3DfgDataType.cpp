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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Dfg.h"
#અન્તર્ભાવ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgDataType

અચલ DfgDataType* DfgDataType::s_nullTypep{nullptr};
std::unordered_map<uint32_t, અચલ DfgDataType*> DfgDataType::s_packedTypes{};

//------------------------------------------------------------------------------
// Type checker - for internal validation only

શૂન્ય V3DfgPasses::typeCheck(અચલ DfgGraph& dfg) {
    dfg.forEachVertex([&](અચલ DfgVertex& vtx) { vtx.typeCheck(dfg); });
}
