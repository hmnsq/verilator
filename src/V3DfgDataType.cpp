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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Dfg.h"
#समावेश "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgDataType

स्थिर Dfg_तथ्य_Type* Dfg_तथ्य_Type::s_nullTypep{nullptr};
std::unordered_map<uint32_t, स्थिर Dfg_तथ्य_Type*> Dfg_तथ्य_Type::s_packedTypes{};

//------------------------------------------------------------------------------
// Type checker - for internal validation only

व्योम V3DfgPasses::typeCheck(स्थिर DfgGraph& dfg) {
    dfg.forEachVertex([&](स्थिर DfgVertex& vtx) { vtx.typeCheck(dfg); });
}
