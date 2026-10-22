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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Dfg.h"
#ಅನ್ತರ್ಭಾವ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgDataType

ಧ್ರುವ DfgDataType* DfgDataType::s_nullTypep{nullptr};
std::unordered_map<uint32_t, ಧ್ರುವ DfgDataType*> DfgDataType::s_packedTypes{};

//------------------------------------------------------------------------------
// Type checker - for internal validation only

ಶೂನ್ಯ V3DfgPasses::typeCheck(ಧ್ರುವ DfgGraph& dfg) {
    dfg.forEachVertex([&](ಧ್ರುವ DfgVertex& vtx) { vtx.typeCheck(dfg); });
}
