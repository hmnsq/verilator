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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Dfg.h"
#अन्तर्भाव "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgDataType

ध्रुव DfgDataType* DfgDataType::s_nullTypep{nullptr};
std::unordered_map<uint32_t, ध्रुव DfgDataType*> DfgDataType::s_packedTypes{};

//------------------------------------------------------------------------------
// Type checker - for internal validation only

शून्य V3DfgPasses::typeCheck(ध्रुव DfgGraph& dfg) {
    dfg.forEachVertex([&](ध्रुव DfgVertex& vtx) { vtx.typeCheck(dfg); });
}
