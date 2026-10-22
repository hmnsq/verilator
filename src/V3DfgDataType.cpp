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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Dfg.h"
#شامل "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgDataType

ثابت Dfg_ڈیٹا_Type* Dfg_ڈیٹا_Type::s_nullTypep{خالی_اشاریہ};
std::unordered_map<uint32_t, ثابت Dfg_ڈیٹا_Type*> Dfg_ڈیٹا_Type::s_packedTypes{};

//------------------------------------------------------------------------------
// Type checker - for internal validation only

خالی V3DfgPasses::typeCheck(ثابت Dfg_گراف& dfg) {
    dfg.for_Each_راس([&](ثابت Dfg_راس& vtx) { vtx.typeCheck(dfg); });
}
