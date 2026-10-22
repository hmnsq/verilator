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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dfg.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgDataType

ਅਚਲ Dfg_ਅੰਕੜੇ_Type* Dfg_ਅੰਕੜੇ_Type::s_nullTypep{ਖਾਲੀ_ਸੂਚਕ};
std::unordered_map<uint32_t, ਅਚਲ Dfg_ਅੰਕੜੇ_Type*> Dfg_ਅੰਕੜੇ_Type::s_packedTypes{};

//------------------------------------------------------------------------------
// Type checker - for internal validation only

ਖਾਲੀ V3DfgPasses::typeCheck(ਅਚਲ Dfg_ਗ੍ਰਾਫ& dfg) {
    dfg.for_Each_ਸ਼ੀਰਸ਼([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) { vtx.typeCheck(dfg); });
}
