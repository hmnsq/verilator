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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Dfg.h"
#ଅନ୍ତର୍ଭାଵ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgDataType

ଅଚଳ DfgDataType* DfgDataType::s_nullTypep{nullptr};
std::unordered_map<uint32_t, ଅଚଳ DfgDataType*> DfgDataType::s_packedTypes{};

//------------------------------------------------------------------------------
// Type checker - for internal validation only

ଶୂନ୍ୟ V3DfgPasses::typeCheck(ଅଚଳ DfgGraph& dfg) {
    dfg.forEachVertex([&](ଅଚଳ DfgVertex& vtx) { vtx.typeCheck(dfg); });
}
