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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Dfg.h"
#ضمن "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgDataType

ثابت Dfg_معلومات_Type* Dfg_معلومات_Type::s_nullTypep{تش_اشاره};
std::unordered_map<uint32_t, ثابت Dfg_معلومات_Type*> Dfg_معلومات_Type::s_packedTypes{};

//------------------------------------------------------------------------------
// Type checker - for internal validation only

فراغ V3DfgPasses::typeCheck(ثابت Dfg_ګراف& dfg) {
    dfg.for_Each_راس([&](ثابت Dfg_راس& vtx) { vtx.typeCheck(dfg); });
}
